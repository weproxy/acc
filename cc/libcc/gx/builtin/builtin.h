//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
// int ...
typedef int8_t int8;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint32_t uint;
typedef uint8_t byte;
}  // namespace gx

#include "R.h"
#include "chan.h"
#include "def.h"
#include "defer.h"
#include "go.h"
#include "slice.h"
#include "util.h"
