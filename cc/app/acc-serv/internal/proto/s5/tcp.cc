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

namespace internal {
namespace proto {
namespace s5 {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////
// handleTCP ...
void handleTCP(net::Conn c, net::Addr raddr) {
    auto tag = GX_SS(TAG << " TCP_" << nx::NewID() << " " << c->RemoteAddr() << "->" << raddr);
    auto sta = stats::NewTCPStats(stats::Type::S5, tag);

    sta->Start("connected");
    DEFER(sta->Done("closed"));

    // dial target server
    AUTO_R(rc, er1, net::Dial(raddr));
    if (er1) {
        LOGS_E(TAG << " dial, err: " << er1);
        socks::WriteReply(c, socks::Reply::HostUnreachable);
        return;
    }
    DEFER(rc->Close());

    // <<< REP:
    //     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |
    auto err = socks::WriteReply(c, socks::Reply::Success, 0, net::MakeAddr(net::IPv4zero, 0));
    if (err) {
        if (err != net::ErrClosed) {
            LOGS_E(TAG << " err: " << err);
        }
        return;
    }

    netio::RelayOption opt(time::Second * 2);
    opt.A2B.CopingFn = [sta](int n) { sta->AddRecv(n); };
    opt.B2A.CopingFn = [sta](int n) { sta->AddSent(n); };

    // Relay c <--> rc
    netio::Relay(c, rc, opt);
}

////////////////////////////////////////////////////////////////////////////////
// handleUDP ...
extern void handleUDP(net::PacketConn c, net::Addr caddr, net::Addr raddr);

// handleAssoc ...
void handleAssoc(net::Conn c, net::Addr raddr) {
    auto caddr = c->RemoteAddr();

    auto tag = GX_SS(TAG << " Assoc_" << nx::NewID() << " " << caddr << "->" << raddr);
    auto sta = stats::NewTCPStats(stats::Type::S5, tag);

    sta->Start("connected");
    DEFER(sta->Done("closed"));

    AUTO_R(ln, er1, net::ListenPacket(":0"));
    if (er1) {
        LOGS_E(TAG << " dial, err: " << er1);
        socks::WriteReply(c, socks::Reply::HostUnreachable);
        return;
    }

    // handleUDP
    gx::go([ln, caddr, raddr] {
        // raddr is not fixed, it maybe be changed after
        handleUDP(ln, caddr, raddr);
    });

    // <<< REP:
    //     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |
    auto err = socks::WriteReply(c, socks::Reply::Success, 0, ln->LocalAddr());
    if (err) {
        if (err != net::ErrClosed) {
            LOGS_E(TAG << " err: " << err);
        }
        return;
    }

    // discard
    io::Copy(io::Discard, c);
}

}  // namespace s5
}  // namespace proto
}  // namespace internal
