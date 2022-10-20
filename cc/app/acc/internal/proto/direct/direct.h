//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

namespace app {
namespace proto {
namespace direct {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[direct]";

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Handler, error> New(const string& servURL);

}  // namespace direct
}  // namespace proto
}  // namespace app
