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
// handler_t ...
struct handler_t : public proto::handler_t {
    handler_t(const string& servURL) {}

    ////////////////////////////////////////////////////////////////////////////////
    // Handle ...
    virtual error Handle(net::Conn c, net::Addr raddr) override { return nil; }

    ////////////////////////////////////////////////////////////////////////////////
    // HandlePacket ...
    virtual error HandlePacket(net::PacketConn c, net::Addr raddr) override { return nil; }

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
    proto::Register({"s5", "s5x"}, New);
    return true;
}();

}  // namespace s5
}  // namespace proto
}  // namespace internal
