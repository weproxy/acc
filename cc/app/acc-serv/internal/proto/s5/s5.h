//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

namespace internal {
namespace proto {
namespace s5 {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[s5]";

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j);

}  // namespace s5
}  // namespace proto
}  // namespace internal
