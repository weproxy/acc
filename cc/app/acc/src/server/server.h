//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "def.h"

namespace app {
namespace server {

// Start ...
std::tuple<io::Closer, error> Start();

}  // namespace server
}  // namespace app
