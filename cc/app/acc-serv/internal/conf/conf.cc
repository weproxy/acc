//
// weproxy@foxmail.com 2022/10/03
//

#include "conf.h"
#include "conf.json.h"
#include "gx/encoding/json/json.h"

namespace internal {
namespace conf {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Iface, in, out)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Conf, iface, server)

// ReadConfig ...
R<json::J, error> ReadConfig() {
    return json::Parse(DEFAULT_CONF);
}

}  // namespace conf
}  // namespace internal
