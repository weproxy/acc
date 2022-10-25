//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

namespace internal {
namespace proto {
namespace gaap {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[gaap]";

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL);

}  // namespace gaap
}  // namespace proto
}  // namespace internal
