//
// weproxy@foxmail.com 2022/10/03
//

#include "fx/fx.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/socks/socks.h"
#include "nx/stats/stats.h"
#include "ss.h"

namespace internal {
namespace proto {
namespace ss {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////
// handleTCP ...
error handleTCP(net::Conn c, net::Addr raddr) {
    auto tag = GX_SS(TAG << " TCP_" << nx::NewID() << " " << c->RemoteAddr() << "->" << raddr);
    auto sta = stats::NewTCPStats(stats::Type::S5, tag);

    sta->Start("connected");
    DEFER(sta->Done("closed"));

    // AUTO_R(rc, er1, net::Dial(raddr));
    // if (er1) {
    //     LOGS_E(TAG << " dial, err: " << er1);
    //     socks::WriteReply(c, socks::ReplyHostUnreachable);
    //     return er1;
    // }

    // auto err = socks::WriteReply(c, socks::ReplySuccess, 0, net::MakeAddr(net::IPv4zero, 0));
    // if (err) {
    //     if (err != net::ErrClosed) {
    //         LOGS_E(TAG << " err: " << err);
    //     }
    //     return err;
    // }

    // AUTO_R(read, written, er2, nx::io::Relay(c, rc, stats::NewRelayOption(sta)));
    // if (er2) {
    //     if (er2 != net::ErrClosed) {
    //         LOGS_E(TAG << " relay " << tag << " , err: " << er2);
    //     }
    //     return er2;
    // }

    return nil;
}

}  // namespace ss
}  // namespace proto
}  // namespace internal
