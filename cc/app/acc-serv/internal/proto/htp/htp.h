//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

namespace app {
namespace proto {
namespace htp {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[htp]";

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j);

}  // namespace htp
}  // namespace proto
}  // namespace app
