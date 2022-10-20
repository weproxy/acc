//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"

namespace nx {
using namespace gx;

// NewID ...
uint64 NewID();

// BindInterface ...
error BindInterface(int fd, int ifaceInex);

}  // namespace nx
