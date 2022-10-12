//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

#define NAMESPACE_BEG_DIRECT \
    namespace app {          \
    namespace proto {        \
    namespace direct {
#define NAMESPACE_END_DIRECT \
    }                        \
    }                        \
    }

NAMESPACE_BEG_DIRECT

////////////////////////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[direct]";

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL);

NAMESPACE_END_DIRECT
