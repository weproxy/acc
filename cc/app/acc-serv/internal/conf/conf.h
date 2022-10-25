//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"
#include "gx/encoding/json/json.h"

namespace internal {
namespace conf {

// ReadConfig ...
R<json::J, error> ReadConfig();

}  // namespace conf
}  // namespace internal
