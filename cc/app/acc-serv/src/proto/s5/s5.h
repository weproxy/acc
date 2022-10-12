//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
#define NAMESPACE_BEG_S5 \
    namespace app {      \
    namespace proto {    \
    namespace s5 {

#define NAMESPACE_END_S5 \
    }                    \
    }                    \
    }
////////////////////////////////////////////////////////////////////////////////////////////////////

NAMESPACE_BEG_S5

////////////////////////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[s5]";

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j);

NAMESPACE_END_S5
