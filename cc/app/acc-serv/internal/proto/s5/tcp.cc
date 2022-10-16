//
// weproxy@foxmail.com 2022/10/03
//

#include "fx/fx.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/netio/netio.h"
#include "nx/socks/socks.h"
#include "nx/stats/stats.h"
#include "s5.h"

namespace app {
namespace proto {
namespace s5 {

namespace xx {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////////////////////////
// handleTCP ...
error handleTCP(net::Conn c, net::Addr raddr) {
    auto tag = GX_SS(TAG << " TCP_" << nx::NewID() << " " << c->RemoteAddr() << "->" << raddr);
    auto sta = stats::NewTCPStats(stats::TypeS5, tag);

    sta->Start("connected");
    DEFER(sta->Done("closed"));

    AUTO_R(rc, er1, net::Dial(raddr));
    if (er1) {
        LOGS_E(TAG << " dial, err: " << er1);
        socks::WriteReply(c, socks::ReplyHostUnreachable);
        return er1;
    }

    // <<< REP:
    //     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |
    auto err = socks::WriteReply(c, socks::ReplySuccess, 0, net::MakeAddr(net::IPv4zero, 0));
    if (err) {
        if (err != net::ErrClosed) {
            LOGS_E(TAG << " err: " << err);
        }
        return err;
    }

    netio::RelayOption opt;
    opt.Read.CopingFn = [sta = sta](int n) { sta->AddRecv(n); };
    opt.Write.CopingFn = [sta = sta](int n) { sta->AddSent(n); };

    AUTO_R(read, written, er2, netio::Relay(c, rc, opt));
    if (er2) {
        if (er2 != net::ErrClosed) {
            LOGS_E(TAG << " relay " << tag << " , err: " << er2);
        }
        return er2;
    }

    return nil;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// handleUDP ...
extern error handleUDP(net::PacketConn c, net::Addr caddr, net::Addr raddr);

// handleAssoc ...
error handleAssoc(net::Conn c, net::Addr raddr) {
    auto caddr = c->RemoteAddr();

    auto tag = GX_SS(TAG << " Assoc_" << nx::NewID() << " " << caddr << "->" << raddr);
    auto sta = stats::NewTCPStats(stats::TypeS5, tag);

    sta->Start("connected");
    DEFER(sta->Done("closed"));

    AUTO_R(ln, er1, net::ListenPacket(":0"));
    if (er1) {
        LOGS_E(TAG << " dial, err: " << er1);
        socks::WriteReply(c, socks::ReplyHostUnreachable);
        return er1;
    }

    // handleUDP
    gx::go([ln = ln, caddr = caddr, raddr = raddr] {
        // raddr is not fixed, it maybe be changed after
        handleUDP(ln, caddr, raddr);
    });

    // <<< REP:
    //     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |
    auto err = socks::WriteReply(c, socks::ReplySuccess, 0, ln->LocalAddr());
    if (err) {
        if (err != net::ErrClosed) {
            LOGS_E(TAG << " err: " << err);
        }
        return err;
    }

    AUTO_R(_, er2, io::Copy(io::Discard, c));
    if (er2) {
        if (er2 != net::ErrClosed) {
            LOGS_E(TAG << " err: " << er2);
        }
        return er2;
    }

    return nil;
}
}  // namespace xx

}  // namespace s5
}  // namespace proto
}  // namespace app
