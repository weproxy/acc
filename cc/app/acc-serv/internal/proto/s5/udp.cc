//
// weproxy@foxmail.com 2022/10/03
//

#include "fx/fx.h"
#include "gx/encoding/binary/binary.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/dns/dns.h"
#include "nx/netio/netio.h"
#include "nx/socks/socks.h"
#include "nx/stats/stats.h"
#include "s5.h"

namespace internal {
namespace proto {
namespace s5 {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////
// key_t ...
typedef uint64 key_t;

// makeKey ...
static key_t makeKey(net::IP ip, uint16 port) {
    uint64 a = (uint64)binary::LittleEndian.Uint32(ip.B);  // ipv4
    uint64 b = (uint64)port;
    return a << 16 | b;
}
static key_t makeKey(net::Addr addr) { return makeKey(addr->IP, addr->Port); }

////////////////////////////////////////////////////////////////////////////////
// udpSess_t ...
//
// <NAT-Open Notice> maybe one source client send to multi-target server, likes:
//   caddr=1.2.3.4:100 --> raddr=8.8.8.8:53
//                     --> raddr=4.4.4.4:53
//
struct udpSess_t : public std::enable_shared_from_this<udpSess_t> {
    net::PacketConn ln_;  // to source client, it is net::PacketConn
    net::PacketConn rc_;  // to target server, it is udpConn_t
    net::Addr caddr_;     // source client addr
    Ref<stats::Stats> sta_;
    func<void()> afterClosedFn_;

    udpSess_t(net::PacketConn ln, net::PacketConn rc, net::Addr caddr) : ln_(ln), rc_(rc), caddr_(caddr) {}
    ~udpSess_t() { Close(); }

    // Close ...
    error Close() {
        if (rc_) {
            rc_->Close();
            sta_->Done("closed");
            rc_ = nil;
            if (afterClosedFn_) {
                afterClosedFn_();
            }
        }
        return nil;
    }

    // Start ...
    void Start() {
        auto tag = GX_SS(TAG << " UDP_" << nx::NewID() << " " << caddr_);
        auto sta = stats::NewUDPStats(stats::Type::S5, tag);

        sta->Start("started");
        sta_ = sta;

        // loopRecvRC
        auto thiz = shared_from_this();
        gx::go([thiz] {
            DEFER(thiz->Close());

            netio::CopyOption opt(time::Minute * 5, 0);
            opt.WriteAddr = thiz->caddr_;
            opt.CopingFn = [thiz](int size) { thiz->sta_->AddSent(size); };

            // copy to ln_ from rc_
            netio::Copy(thiz->ln_, thiz->rc_, opt);
        });
    }

    // WriteToRC ...
    // unpackedBuf is from source client
    error WriteToRC(const bytez<> unpackedBuf, net::Addr raddr) {
        if (!rc_) {
            return net::ErrClosed;
        }

        sta_->AddRecv(len(unpackedBuf));

        // udpConn_t::WriteTo target server
        AUTO_R(_, err, rc_->WriteTo(unpackedBuf, raddr));

        return err;
    }
};

// udpSessMap ...
using udpSessMap = Map<key_t, Ref<udpSess_t>>;

////////////////////////////////////////////////////////////////////////////////
// udpConn_t
// to target server
struct udpConn_t : public net::xx::packetConnWrap_t {
    socks::AddrType typ_{socks::AddrType::IPv4};

    udpConn_t(net::PacketConn pc) : net::xx::packetConnWrap_t(pc) {}

    // ReadFrom ...
    // readFrom target server, and pack data (to client)
    //
    // <<< RSP:
    //     | RSV | FRAG | ATYP | SRC.ADDR | SRC.PORT | DATA |
    //     +-----+------+------+----------+----------+------+
    //     |  2  |  1   |  1   |    ...   |    2     |  ... |
    virtual R<int, net::Addr, error> ReadFrom(bytez<> buf) override {
        if (socks::AddrType::IPv4 != typ_ && socks::AddrType::IPv6 != typ_) {
            return {0, nil, socks::ErrInvalidAddrType};
        }

        // readFrom target server
        int pos = 3 + 1 + (socks::AddrType::IPv4 == typ_ ? 4 : 16) + 2;
        AUTO_R(n, addr, err, wrap_->ReadFrom(buf(pos)));
        if (err) {
            return {n, addr, err};
        }

        // dns cache store
        if (addr->Port == 53) {
            dns::OnResponse(buf(pos, pos + n));
        }

        // pack data
        auto raddr = socks::FromNetAddr(addr);
        copy(buf(3), raddr->B);
        buf[0] = buf[1] = buf[2] = 0;  // RSV|FRAG

        return {3 + len(raddr->B) + n, addr, nil};
    }

    // WriteTo ...
    // unpackedBuf from source client, and writeTo target server
    virtual R<int, error> WriteTo(const bytez<> unpackedBuf, net::Addr raddr) override {
        // writeTo target server
        return wrap_->WriteTo(unpackedBuf, raddr);
    }

    // wrap ...
    static net::PacketConn wrap(net::PacketConn pc) {
        // wrap
        return NewRef<udpConn_t>(pc);
    }
};

////////////////////////////////////////////////////////////////////////////////
// handleUDP ...
//
// <NAT-Open Notice> maybe one source client send to multi-target server, likes:
//   caddr=1.2.3.4:100 --> raddr=8.8.8.8:53
//                     --> raddr=4.4.4.4:53
//
error handleUDP(net::PacketConn ln, net::Addr caddr, net::Addr raddr) {
    // sessMap
    auto sessMap = NewRef<udpSessMap>();
    DEFER({
        for (auto& kv : *sessMap) {
            kv.second->Close();
        }
    });

    DEFER(ln->Close());

    bytez<> buf = make(1024 * 8);

    for (;;) {
        // 5 minutes timeout
        ln->SetReadDeadline(time::Now().Add(time::Minute * 5));

        // read
        AUTO_R(n, caddr, err, ln->ReadFrom(buf));
        if (err) {
            if (err != net::ErrClosed) {
                LOGS_E(TAG << " err: " << err);
            }
            break;
        } else if (n <= 10) {
            continue;
        }

        // packet data
        auto data = buf(0, n);

        //
        // >>> REQ:
        //     | RSV | FRAG | ATYP | DST.ADDR | DST.PORT | DATA |
        //     +-----+------+------+----------+----------+------+
        //     |  2  |  1   |  1   |    ...   |    2     |  ... |
        AUTO_R(saddr, er1, socks::ParseAddr(data(3)));
        if (er1) {
            continue;
        }

        auto raddr = saddr->ToNetAddr();
        data = data(3 + len(saddr->B));

        // dns cache query
        if (raddr->Port == 53) {
            AUTO_R(msg, ans, erx, nx::dns::OnRequest(data));
            if (!erx && ans) {
                // dns cache answer
                auto b = ans->Bytes();
                if (len(b) > 0) {
                    buf[0] = buf[1] = buf[2] = 0;  // RSV|FRAG
                    int off = 3 + len(saddr->B);
                    copy(buf(off), b);
                    ln->WriteTo(buf(0, off + len(b)), caddr);
                    continue;
                }
            }
        }

        // lookfor or create sess
        Ref<udpSess_t> sess;

        // use source client addr as key
        // <TODO-Notice>
        //  some user's env has multi-outbound-ip, we will get diff caddr although he use one-same-conn
        key_t key = makeKey(caddr);

        auto it = sessMap->find((key));
        if (it == sessMap->end()) {
            // create a new rc for every new source client
            AUTO_R(c, err, net::ListenPacket(":0"));
            if (err) {
                LOGS_E(TAG << " err: " << err);
                continue;
            }

            // wrap as udpConn_t
            auto rc = udpConn_t::wrap(c);

            // store to sessMap
            sess = NewRef<udpSess_t>(ln, rc, caddr);
            (*sessMap)[key] = sess;

            // remove it after rc closed
            sess->afterClosedFn_ = [sessMap, key] { sessMap->erase(key); };

            // start recv loop...
            sess->Start();
        } else {
            sess = it->second;
        }

        // writeTo target server
        err = sess->WriteToRC(data, raddr);
        if (err) {
            return err;
        }
    }

    return nil;
}

}  // namespace s5
}  // namespace proto
}  // namespace internal
