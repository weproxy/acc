//
// weproxy@foxmail.com 2022/10/03
//

#include "htp.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"

namespace app {
namespace proto {
namespace htp {

// handleProxy handle CONNECT command
error handleProxy(net::Conn c, slice<byte> hdr) {
    return gx_TodoErr();
}

}  // namespace htp
}  // namespace proto
}  // namespace app
