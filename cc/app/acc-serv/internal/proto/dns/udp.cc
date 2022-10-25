//
// weproxy@foxmail.com 2022/10/03
//

#include "dns.h"
#include "gx/encoding/binary/binary.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/dns/dns.h"
#include "nx/netio/netio.h"
#include "nx/stats/stats.h"

namespace internal {
namespace proto {
namespace dns {
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
    void Start(const string& target) {
        auto tag = GX_SS(TAG << " UDP_" << nx::NewID() << " " << caddr_ << "->" << target);
        auto sta = stats::NewUDPStats(stats::Type::DNS, tag);

        sta->Start("started");
        sta_ = sta;

        // loopRecvRC
        auto thiz = shared_from_this();
        gx::go([thiz] {
            DEFER(thiz->Close());

            netio::CopyOption opt(time::Second * 10, 0);
            opt.WriteAddr = thiz->caddr_;
            opt.MaxTimes = 1;  // read 1 packet then close
            opt.ReadTimeout = time::Second * 5;
            opt.CopingFn = [thiz](int size) { thiz->sta_->AddSent(size); };

            // copy to ln_ from rc_
            netio::Copy(thiz->ln_, thiz->rc_, opt);
        });
    }

    // WriteToRC ...
    error WriteToRC(const bytez<> buf, net::Addr raddr) {
        if (!rc_) {
            return net::ErrClosed;
        }

        // buf is from source client
        sta_->AddRecv(len(buf));

        AUTO_R(_, err, rc_->WriteTo(buf, raddr));

        return err;
    }
};

// udpSessMap ...
using udpSessMap = Map<key_t, Ref<udpSess_t>>;

////////////////////////////////////////////////////////////////////////////////
// udpConn_t
// to target server
struct udpConn_t : public net::xx::packetConnWrap_t {
    udpConn_t(net::PacketConn pc) : net::xx::packetConnWrap_t(pc) {}

    // ReadFrom ...
    // readFrom target server, and writeTo source client
    virtual R<int, net::Addr, error> ReadFrom(bytez<> buf) override {
        // readFrom target server
        AUTO_R(n, addr, err, wrap_->ReadFrom(buf));
        if (err) {
            return {n, addr, err};
        }

        // cache store
        nx::dns::OnResponse(buf(0, n));

        return {n, addr, nil};
    }

    // WriteTo ...
    // data from source client writeTo target server
    virtual R<int, error> WriteTo(const bytez<> buf, net::Addr raddr) override {
        // TODO ... unpack DNS query packet

        AUTO_R(n, err, wrap_->WriteTo(buf, raddr));

        return {n, err};
    }

    // wrap ...
    static net::PacketConn wrap(net::PacketConn pc) {
        //
        return NewRef<udpConn_t>(pc);
    }
};

////////////////////////////////////////////////////////////////////////////////
// runServLoop ...
error runServLoop(net::PacketConn ln) {
    // sessMap
    auto sessMap = NewRef<udpSessMap>();
    DEFER({
        for (auto& kv : *sessMap) {
            kv.second->Close();
        }
    });

    DEFER(ln->Close());

    bytez<> buf = make(1024 * 4);

    for (;;) {
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

        LOGS_V(TAG << " ReadFrom() " << caddr);

        // packet data
        auto data = buf(0, n);

        // cache query
        AUTO_R(msg, ans, erx, nx::dns::OnRequest(data));
        if (!erx && ans) {
            // cache answer
            auto b = ans->Bytes();
            if (len(b) > 0) {
                ln->WriteTo(b, caddr);
                continue;
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

            auto target = [msg]() -> string {
                if (msg != nil && len(msg->Questions) > 0) {
                    return GX_SS(msg->Questions[0].Name << msg->Questions[0].Type);
                }
                return "";
            }();

            // start recv loop...
            sess->Start(target);
        } else {
            sess = it->second;
        }

        // use 8.8.8.8:53
        static net::Addr raddr = net::MakeAddr(net::IPv4(223, 5, 5, 5), 53);

        // writeTo target server
        err = sess->WriteToRC(data, raddr);
        if (err) {
            return err;
        }
    }

    return nil;
}

}  // namespace dns
}  // namespace proto
}  // namespace internal
