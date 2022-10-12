//
// weproxy@foxmail.com 2022/10/03
//

#include "gaap.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/socks/socks.h"

NAMESPACE_BEG_GAAP
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
    void Close() override { LOGS_D(TAG << " Close()"); }
};
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL) {
    LOGS_V(TAG << " servURL: " << servURL);

    auto s = std::shared_ptr<xx::Handler>(new xx::Handler(servURL));

    return {s, nil};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    // LOGS_V(TAG << " init()");
    proto::Register("gaap", New);
    return true;
}();

NAMESPACE_END_GAAP
