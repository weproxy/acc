//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/bytes/bytes.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "htp.h"
#include "logx/logx.h"

namespace internal {
namespace proto {
namespace htp {

// handleProxy handle reverse-proxy mode
error handleProxy(net::Conn c, bytez<> hdr) { return gx_TodoErr(); }

}  // namespace htp
}  // namespace proto
}  // namespace internal
