//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

#define NAMESPACE_BEG_HTP \
    namespace app {       \
    namespace proto {     \
    namespace htp {
#define NAMESPACE_END_HTP \
    }                     \
    }                     \
    }

NAMESPACE_BEG_HTP

////////////////////////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[htp]";

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL);

NAMESPACE_END_HTP
