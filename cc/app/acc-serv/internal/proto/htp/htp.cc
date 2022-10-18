//
// weproxy@foxmail.com 2022/10/03
//

#include "htp.h"

#include "../proto.h"
#include "gx/net/net.h"
#include "logx/logx.h"

namespace app {
namespace proto {
namespace htp {

namespace xx {
////////////////////////////////////////////////////////////////////////////////////////////////////
// handleConn ...
static error handleConn(net::Conn c) {
    DEFER(c->Close());

    return nil;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Server ...
struct Server : public proto::IServer {
    string addr_;
    net::Listener ln_;

    Server(const string& addr) : addr_(addr) {}

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //  Start ...
    error Start() override {
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

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Close ...
    error Close() override {
        if (ln_) {
            ln_->Close();
        }
        LOGS_D(TAG << " Close()");
        return nil;
    }
};
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j) {
    LOGS_V(TAG << " Conf: " << j);

    auto addr = j["listen"];
    if (!addr.is_string() || addr.empty()) {
        return {nil, errors::New("invalid addr")};
    }

    return {MakeRef<xx::Server>(string(addr)), nil};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    // LOGS_V(TAG << " init()");
    proto::Register("htp", New);
    return true;
}();

}  // namespace htp
}  // namespace proto
}  // namespace app
