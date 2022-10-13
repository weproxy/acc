//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"

namespace biz {
using namespace gx;
namespace report {

// Report ...
error Report(const string& msg);

// Track ...
error Track(const string& msg);

}  // namespace report
}  // namespace biz
