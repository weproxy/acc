//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

#define NAMESPACE_BEG_GAAP \
    namespace app {        \
    namespace proto {      \
    namespace gaap {
#define NAMESPACE_END_GAAP \
    }                      \
    }                      \
    }

NAMESPACE_BEG_GAAP

////////////////////////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[gaap]";

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL);

NAMESPACE_END_GAAP
