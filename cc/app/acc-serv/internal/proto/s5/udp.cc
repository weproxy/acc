//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/socks/socks.h"
#include "s5.h"

NAMESPACE_BEG_S5

namespace xx {
using namespace nx;

// handleUDP ...
error handleUDP(net::Conn c, net::Addr raddr) {
    AUTO_R(_, err, io::Copy(c, c));
    return err;
}

}  // namespace xx

NAMESPACE_END_S5
