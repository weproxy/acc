//
// weproxy@foxmail.com 2022/10/03
//

#include "ss.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"

namespace app {
namespace proto {
namespace ss {

extern error handleTCP(net::Conn c, net::Addr raddr);
extern error handleUDP(net::PacketConn pc, net::Addr caddr, const slice<byte> buf);

////////////////////////////////////////////////////////////////////////////////
// handleConn ...
static error handleConn(net::Conn c) {
    DEFER(c->Close());

    return handleTCP(c, nil);
}

////////////////////////////////////////////////////////////////////////////////
// server_t ...
struct server_t : public proto::server_t {
    string addr_;
    net::Listener ln_;
    net::PacketConn pc_;

    server_t(const string& addr) : addr_(addr) {}

    ////////////////////////////////////////////////////////////////////////////////
    //  Start ...
    virtual error Start() override {
        AUTO_R(ln, err, net::Listen(addr_));
        if (err) {
            LOGS_E(TAG << " Start(" << addr_ << "), err: " << err);
            return err;
        }

        AUTO_R(pc, er2, net::ListenPacket(addr_));
        if (er2) {
            LOGS_E(TAG << " Start(" << addr_ << "), err: " << er2);
            return er2;
        }

        LOGS_D(TAG << " Start(" << addr_ << ")");

        ln_ = ln;
        pc_ = pc;

        gx::go([ln] {
            for (;;) {
                AUTO_R(c, err, ln->Accept());
                if (err) {
                    LOGS_E(TAG << " accept(), err: " << err);
                    break;
                }

                LOGS_V(TAG << " accept() " << c->RemoteAddr());

                gx::go([c] { handleConn(c); });
            }
        });

        gx::go([pc] {
            slice<byte> buf = make(1024 * 8);

            for (;;) {
                AUTO_R(n, caddr, err, pc->ReadFrom(buf));
                if (err) {
                    LOGS_E(TAG << " readFrom(), err: " << err);
                    break;
                }

                if (n > 0) {
                    LOGS_D(TAG << " recvfrom(" << caddr << ") " << n << " bytes");
                    handleUDP(pc, caddr, buf(0, n));
                }
            }
        });

        return nil;
    }

    // Close ...
    virtual error Close() override {
        if (ln_) {
            ln_->Close();
        }
        if (pc_) {
            pc_->Close();
        }

        LOGS_D(TAG << " Close()");
        return nil;
    }
};

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j) {
    LOGS_V(TAG << " Conf: " << j);

    auto addr = j["listen"];
    if (!addr.is_string() || addr.empty()) {
        return {nil, errors::New("invalid addr")};
    }

    return {NewRef<server_t>(string(addr)), nil};
}

////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    // LOGS_V(TAG << " init()");
    proto::Register("ss", New);
    return true;
}();

}  // namespace ss
}  // namespace proto
}  // namespace app
