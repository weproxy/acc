//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/socks/socks.h"
#include "ss.h"

namespace internal {
namespace proto {
namespace ss {
using namespace nx;

// handleUDP ...
void handleUDP(net::PacketConn pc, net::Addr caddr, const bytez<> buf) {
    // AUTO_R(_, err, io::Copy(c, c));
    AUTO_R(n, err, pc->WriteTo(buf, caddr));
    if (err) {
        LOGS_D(TAG << " writeTo(" << caddr << ") " << n << " bytes, err: " << err);
    } else {
        LOGS_D(TAG << " writeTo(" << caddr << ") " << n << " bytes");
    }
}

}  // namespace ss
}  // namespace proto
}  // namespace internal
