//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

namespace internal {
namespace proto {
namespace kc {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[kc]";

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j);

}  // namespace kc
}  // namespace proto
}  // namespace internal
