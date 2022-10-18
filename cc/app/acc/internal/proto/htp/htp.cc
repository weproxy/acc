//
// weproxy@foxmail.com 2022/10/03
//

#include "htp.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/socks/socks.h"

NAMESPACE_BEG_HTP
using namespace nx;

namespace xx {
////////////////////////////////////////////////////////////////////////////////////////////////////
// Handler ...
struct Handler : public proto::IHandler {
    Handler(const string& servURL) {}

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Handle ...
    virtual error Handle(net::Conn c, net::Addr raddr) override { return nil; }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // HandlePacket ...
    virtual error HandlePacket(net::PacketConn c, net::Addr raddr) override { return nil; }

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
    return {MakeRef<xx::Handler>(servURL), nil};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    // LOGS_V(TAG << " init()");
    proto::Register("htp", New);
    return true;
}();

NAMESPACE_END_HTP
