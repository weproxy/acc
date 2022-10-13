//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/builtin/builtin.h"

namespace gx {
namespace fmt {

// Sprintf ...
string Sprintf(const string& fmt, ...);

// Errorf ...
error Errorf(const string& fmt, ...);

}  // namespace fmt
}  // namespace gx
