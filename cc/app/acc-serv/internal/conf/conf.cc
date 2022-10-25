//
// weproxy@foxmail.com 2022/10/03
//

#include "conf.h"
#include "conf.json.h"

namespace internal {
namespace conf {

// ReadConfig ...
R<json::J, error> ReadConfig() {
    return json::Parse(DEFAULT_CONF);
}

}  // namespace conf
}  // namespace internal
