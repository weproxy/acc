//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
#define NAMESPACE_BEG_SS \
    namespace app {      \
    namespace proto {    \
    namespace ss {

#define NAMESPACE_END_SS \
    }                    \
    }                    \
    }
////////////////////////////////////////////////////////////////////////////////////////////////////

NAMESPACE_BEG_SS
using namespace nx;

////////////////////////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[ss]";

NAMESPACE_END_SS
