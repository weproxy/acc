//
// weproxy@foxmail.com 2022/10/03
//

#include "s5.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/socks/socks.h"

namespace internal {
namespace proto {
namespace s5 {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////

// checkUserPass ...
static error checkUserPass(const string& user, const string& pass) {
    LOGS_D(TAG << " handshake() auth: user=" << user << ", pass=" << pass);
    return nil;
}

////////////////////////////////////////////////////////////////////////////////
// handleTCP ...
extern error handleTCP(net::Conn c, net::Addr raddr);

// handleAssoc ...
extern error handleAssoc(net::Conn c, net::Addr raddr);

////////////////////////////////////////////////////////////////////////////////
// handleConn ...
static error handleConn(net::Conn c) {
    DEFER(c->Close());

    AUTO_R(cmd, raddr, err, socks::ServerHandshake(c, checkUserPass));
    if (err || !raddr) {
        auto er = err ? err : socks::ErrInvalidAddrType;
        LOGS_E(TAG << " handshake(), err: " << er);
        return er;
    }

    // LOGS_D(TAG << " handshake(), cmd=" << socks::ToString(cmd) << ", raddr=" << raddr);

    switch (cmd) {
        case socks::Command::Connect:
            return handleTCP(c, raddr->ToNetAddr());
        case socks::Command::Associate:
            return handleAssoc(c, raddr->ToNetAddr());
        case socks::Command::Bind:
            return errors::New("not support socks command: bind");
        default:
            return fmt::Errorf("unknow socks command: %d", cmd);
    }
}

////////////////////////////////////////////////////////////////////////////////
// server_t ...
struct server_t : public proto::server_t {
    string addr_;
    net::Listener ln_;

    server_t(const string& addr) : addr_(addr) {}

    ////////////////////////////////////////////////////////////////////////////////
    //  Start ...
    virtual error Start() override {
        AUTO_R(ln, err, net::Listen(addr_));
        if (err) {
            LOGS_E(TAG << " Listen(" << addr_ << "), err: " << err);
            return err;
        }

        LOGS_D(TAG << " Start(" << addr_ << ")");

        ln_ = ln;
        gx::go([ln] {
            for (;;) {
                AUTO_R(c, err, ln->Accept());
                if (err) {
                    if (err != net::ErrClosed) {
                        LOGS_E(TAG << " Accept(), err: " << err);
                    }
                    break;
                }

                LOGS_V(TAG << " Accept() " << c->RemoteAddr());

                gx::go([c] { handleConn(c); });
            }
        });

        return nil;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Close ...
    virtual error Close() override {
        if (ln_) {
            ln_->Close();
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
    proto::Register("s5", New);
    return true;
}();

}  // namespace s5
}  // namespace proto
}  // namespace internal
