//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

namespace internal {
namespace proto {
namespace htp {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[htp]";

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL);

}  // namespace htp
}  // namespace proto
}  // namespace internal
