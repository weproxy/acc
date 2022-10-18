//
// weproxy@foxmail.com 2022/10/03
//

#include "direct.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/socks/socks.h"

NAMESPACE_BEG_DIRECT
using namespace nx;

namespace xx {
////////////////////////////////////////////////////////////////////////////////////////////////////
// Handler ...
struct Handler : public proto::IHandler {
    Handler(const string& servURL) {}

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Handle ...
    virtual error Handle(net::Conn c, net::Addr raddr) { return nil; }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // HandlePacket ...
    virtual error HandlePacket(net::PacketConn c, net::Addr raddr) { return nil; }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Close ...
    error Close() override {
        LOGS_D(TAG << " Close()");
        return nil;
    }
};
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL) {
    LOGS_V(TAG << " servURL: " << servURL);

    auto s = MakeRef<xx::Handler>(servURL);

    return {s, nil};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    // LOGS_V(TAG << " init()");
    proto::Register("direct", New);
    return true;
}();

NAMESPACE_END_DIRECT
