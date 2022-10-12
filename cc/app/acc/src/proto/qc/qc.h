//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

#define NAMESPACE_BEG_QC \
    namespace app {      \
    namespace proto {    \
    namespace qc {
#define NAMESPACE_END_QC \
    }                    \
    }                    \
    }

NAMESPACE_BEG_QC

////////////////////////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[qc]";

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL);

NAMESPACE_END_QC
