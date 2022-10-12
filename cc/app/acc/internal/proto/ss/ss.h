//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

#define NAMESPACE_BEG_SS \
    namespace app {      \
    namespace proto {    \
    namespace ss {
#define NAMESPACE_END_SS \
    }                    \
    }                    \
    }

NAMESPACE_BEG_SS

////////////////////////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[ss]";

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL);

NAMESPACE_END_SS
