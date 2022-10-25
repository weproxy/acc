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
R<proto::Handler, error> New(const string& servURL);

}  // namespace kc
}  // namespace proto
}  // namespace internal
