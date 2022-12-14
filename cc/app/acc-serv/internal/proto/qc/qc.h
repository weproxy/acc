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
R<proto::Server, error> New(const json::J& j);

}  // namespace qc
}  // namespace proto
}  // namespace internal
