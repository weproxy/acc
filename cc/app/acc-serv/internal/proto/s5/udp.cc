//
// weproxy@foxmail.com 2022/10/03
//

#include "fx/fx.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/netio/netio.h"
#include "nx/socks/socks.h"
#include "nx/stats/stats.h"
#include "s5.h"

namespace app {
namespace proto {
namespace s5 {

namespace xx {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////////////////////////
// udpSessPtr ...
struct udpSess_t;
typedef std::shared_ptr<udpSess_t> udpSessPtr;

// key_t ...
typedef uint64 key_t;

// makeKey ...
static key_t makeKey(net::IP ip, uint16 port) {
    uint64 a = (uint64)(*(uint32*)ip.B.data());  // Only IPv4
    uint64 b = (uint64)(*(uint16*)&port);
    return a << 16 | b;
}
static key_t makeKey(net::Addr addr) { return makeKey(addr->IP, addr->Port); }

// udpSessMapPtr ...
typedef Map<key_t, udpSessPtr> udpSessMap;
typedef std::shared_ptr<udpSessMap> udpSessMapPtr;

////////////////////////////////////////////////////////////////////////////////////////////////////
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
    stats::Stats sta_;
    std::function<void()> afterClosedFn_;

    udpSess_t(net::PacketConn ln, net::PacketConn rc, net::Addr caddr) : ln_(ln), rc_(rc), caddr_(caddr) {}
    ~udpSess_t() { Close(); }

    // Close ...
    void Close() {
        if (rc_) {
            rc_->Close();
            if (afterClosedFn_) {
                afterClosedFn_();
            }
            sta_->Done("closed");
            rc_ = nil;
        }
    }

    // Start ...
    void Start() {
        auto tag = GX_SS(TAG << " UDP_" << nx::NewID() << " " << caddr_);
        auto sta = stats::NewUDPStats(stats::TypeS5, tag);

        sta->Start("started");
        sta_ = sta;

        // loopRecvRC
        gx::go([thiz = shared_from_this()] {
            DEFER(thiz->Close());

            slice<byte> buf = make(1024 * 4);

            for (;;) {
                // 5 minutes timeout
                thiz->rc_->SetReadDeadline(time::Now().Add(time::Minute * 5));

                // udpConn_t::ReadFrom()
                // readFrom target server, and pack data (to client)
                AUTO_R(nr, _, er1, thiz->rc_->ReadFrom(buf));
                if (er1) {
                    break;
                } else if (nr > 0) {
                    AUTO_R(nw, er2, thiz->ln_->WriteTo(buf(0, nr), thiz->caddr_));
                    if (er2) {
                        break;
                    } else if (nw > 0) {
                        thiz->sta_->AddSent(nw);
                    }
                }
            }
        });
    }

    // WriteToRC ...
    void WriteToRC(slice<byte> buf) {
        if (!rc_) {
            return;
        }

        // buf is from source client
        sta_->AddRecv(len(buf));

        // udpConn_t::WriteTo()
        // unpack data (from source client), and writeTo target server
        AUTO_R(n, err, rc_->WriteTo(buf, nil));
        if (err) {
            return;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// udpConn_t
// to target server
struct udpConn_t : public net::xx::packetConnWrap_t {
    socks::AddrType typ_{socks::AddrTypeIPv4};

    udpConn_t(net::PacketConn pc) : net::xx::packetConnWrap_t(pc) {}

    // ReadFrom ...
    // readFrom target server, and pack data (to client)
    //
    // <<< RSP:
    //     | RSV | FRAG | ATYP | SRC.ADDR | SRC.PORT | DATA |
    //     +-----+------+------+----------+----------+------+
    //     |  2  |  1   |  1   |    ...   |    2     |  ... |
    virtual R<int, net::Addr, error> ReadFrom(slice<byte> buf) override {
        if (socks::AddrTypeIPv4 != typ_ && socks::AddrTypeIPv6 != typ_) {
            return {0, nil, socks::ErrInvalidAddrType};
        }

        // readFrom target server
        int pos = 3 + 1 + (socks::AddrTypeIPv4 == typ_ ? 4 : 16) + 2;
        AUTO_R(n, addr, err, wrap_->ReadFrom(buf(pos)));
        if (err) {
            return {n, addr, err};
        }

        // pack data

        auto raddr = socks::FromNetAddr(addr);
        socks::CopyAddr(buf(3), raddr);

        n += 3 + len(raddr->B);

        buf[0] = 0;  // RSV
        buf[1] = 0;  //
        buf[2] = 0;  // FRAG

        return {n, addr, nil};
    }

    // WriteTo ...
    // unpack data (from source client), and writeTo target server
    //
    // >>> REQ:
    //     | RSV | FRAG | ATYP | DST.ADDR | DST.PORT | DATA |
    //     +-----+------+------+----------+----------+------+
    //     |  2  |  1   |  1   |    ...   |    2     |  ... |
    virtual R<int, error> WriteTo(const slice<byte> buf, net::Addr) override {
        if (len(buf) < 10) {
            return {0, socks::ErrInvalidSocksVersion};
        }

        // unpack data

        AUTO_R(raddr, er1, socks::ParseAddr(buf(3)));
        if (er1) {
            return {0, er1};
        }

        typ_ = socks::AddrType(raddr->B[0]);
        if (socks::AddrTypeIPv4 != typ_ && socks::AddrTypeIPv6 != typ_) {
            return {0, socks::ErrInvalidAddrType};
        }

        // writeTo target server
        int pos = 3 + len(raddr->B);
        AUTO_R(n, er2, wrap_->WriteTo(buf(pos), raddr->ToNetAddr()));
        if (er2) {
            return {n, er2};
        }

        return {n + pos, nil};
    }

    // wrap ...
    static net::PacketConn wrap(net::PacketConn pc) {
        //
        return std::shared_ptr<udpConn_t>(new udpConn_t(pc));
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// handleUDP ...
//
// <NAT-Open Notice> maybe one source client send to multi-target server, likes:
//   caddr=1.2.3.4:100 --> raddr=8.8.8.8:53
//                     --> raddr=4.4.4.4:53
//
error handleUDP(net::PacketConn ln, net::Addr caddr, net::Addr raddr) {
    // sessMap
    udpSessMapPtr sessMap(new udpSessMap());
    DEFER(for (auto& kv : *sessMap) { kv.second->Close(); });

    DEFER(ln->Close());

    slice<byte> buf = make(1024 * 4);

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
        } else if (n <= 0) {
            continue;
        }

        // packet data
        auto data = buf(0, n);

        // lookfor or create sess
        udpSessPtr sess;

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
            sess = udpSessPtr(new udpSess_t(ln, rc, caddr));
            (*sessMap)[key] = sess;

            // remove it after rc closed
            sess->afterClosedFn_ = [sessMap, key = key] { sessMap->erase(key); };

            // start recv loop...
            sess->Start();
        } else {
            sess = it->second;
        }

        // writeTo target server
        sess->WriteToRC(data);
    }

    return nil;
}

}  // namespace xx

}  // namespace s5
}  // namespace proto
}  // namespace app
