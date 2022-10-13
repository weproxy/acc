//
// weproxy@foxmail.com 2022/10/03
//

#include "fx/fx.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/nx.h"
#include "nx/socks/socks.h"
#include "nx/stats/stats.h"
#include "ss.h"

NAMESPACE_BEG_SS

namespace xx {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////////////////////////
// handleTCP ...
error handleTCP(net::Conn c, net::Addr raddr) {
    auto tag = FX_SS(TAG << " TCP_" << nx::NewID() << " " << c->RemoteAddr() << "->" << raddr);
    auto sta = stats::NewTCPStats(stats::TypeS5, tag);

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

    // AUTO_R(read, written, er2, io::Relay(c, rc, stats::NewRelayOption(sta)));
    // if (er2) {
    //     if (er2 != net::ErrClosed) {
    //         LOGS_E(TAG << " relay " << tag << " , err: " << er2);
    //     }
    //     return er2;
    // }

    return nil;
}

}  // namespace xx

NAMESPACE_END_SS