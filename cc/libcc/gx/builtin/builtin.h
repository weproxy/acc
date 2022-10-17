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

typedef int32 rune;
}  // namespace gx

// gx_TodoErr ...
#define gx_TodoErr() errors::New("<TODO> %s:%d %s", __FILE__, __LINE__, __FUNCTION__)

#include "R.h"
#include "chan.h"
#include "def.h"
#include "defer.h"
#include "go.h"
#include "slice.h"
#include "map.h"
#include "util.h"
