//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"
#include "gx/json/json.h"

namespace app {
namespace conf {

// ReadConfig ...
R<json::J, error> ReadConfig();

}  // namespace conf
}  // namespace app
