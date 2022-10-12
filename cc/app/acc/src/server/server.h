//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"
#include "gx/io/io.h"
#include "gx/errors/errors.h"

namespace app {
namespace server {

// Start ...
R<io::Closer, error> Start();

}  // namespace server
}  // namespace app
