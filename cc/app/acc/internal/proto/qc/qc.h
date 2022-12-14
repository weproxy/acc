//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

namespace internal {
namespace proto {
namespace qc {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[qc]";

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL);

}  // namespace qc
}  // namespace proto
}  // namespace internal
