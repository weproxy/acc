//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

namespace app {
namespace proto {
namespace dns {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[dns]";

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j);

}  // namespace dns
}  // namespace proto
}  // namespace app
