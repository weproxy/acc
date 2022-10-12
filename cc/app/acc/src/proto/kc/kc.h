//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

#define NAMESPACE_BEG_KC \
    namespace app {      \
    namespace proto {    \
    namespace kc {
#define NAMESPACE_END_KC \
    }                    \
    }                    \
    }

NAMESPACE_BEG_KC

////////////////////////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[kc]";

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL);

NAMESPACE_END_KC
