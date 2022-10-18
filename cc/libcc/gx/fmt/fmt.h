//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"

namespace gx {
namespace fmt {

// Sprintf ...
string Sprintf(const char* fmt, ...);

// Errorf ...
error Errorf(const char* fmt, ...);

}  // namespace fmt
}  // namespace gx
