//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

namespace app {
namespace proto {
namespace ss {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[ss]";

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j);

}  // namespace ss
}  // namespace proto
}  // namespace app
