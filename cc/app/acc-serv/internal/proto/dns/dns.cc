//
// weproxy@foxmail.com 2022/10/03
//

#include "dns.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"

namespace app {
namespace proto {
namespace dns {

// runServLoop ...
extern void runServLoop(net::PacketConn ln);

////////////////////////////////////////////////////////////////////////////////////////////////////
// server_t ...
struct server_t : public proto::server_t {
    string addr_;
    net::PacketConn ln_;

    server_t(const string& addr) : addr_(addr) {}

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //  Start ...
    virtual error Start() override {
        AUTO_R(ln, err, net::ListenPacket(addr_));
        if (err) {
            LOGS_E(TAG << " Listen(" << addr_ << "), err: " << err);
            return err;
        }

        LOGS_D(TAG << " Start(" << addr_ << ")");

        ln_ = ln;
        gx::go([ln] { runServLoop(ln); });

        return nil;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Close ...
    virtual error Close() override {
        if (ln_) {
            ln_->Close();
        }
        LOGS_D(TAG << " Close()");
        return nil;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j) {
    LOGS_V(TAG << " Conf: " << j);

    auto addr = j["listen"];
    if (!addr.is_string() || addr.empty()) {
        return {nil, errors::New("invalid addr")};
    }

    return {NewRef<server_t>(string(addr)), nil};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    // LOGS_V(TAG << " init()");
    proto::Register("dns", New);
    return true;
}();

}  // namespace dns
}  // namespace proto
}  // namespace app
