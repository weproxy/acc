//
// weproxy@foxmail.com 2022/10/03
//

#include "htp.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/socks/socks.h"

namespace internal {
namespace proto {
namespace htp {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////
// handler_t ...
struct handler_t : public proto::handler_t {
    handler_t(const string& servURL) {}

    ////////////////////////////////////////////////////////////////////////////////
    // Handle ...
    virtual void Handle(net::Conn c, net::Addr raddr) override {}

    ////////////////////////////////////////////////////////////////////////////////
    // HandlePacket ...
    virtual void HandlePacket(net::PacketConn pc, net::Addr raddr) override {}

    ////////////////////////////////////////////////////////////////////////////////
    // Close ...
    error Close() override {
        LOGS_D(TAG << " Close()");
        return nil;
    }
};

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL) {
    LOGS_V(TAG << " servURL: " << servURL);
    return {NewRef<handler_t>(servURL), nil};
}

////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    proto::Register({"htp", "cdn", "cdnx"}, New);
    return true;
}();

}  // namespace htp
}  // namespace proto
}  // namespace internal
