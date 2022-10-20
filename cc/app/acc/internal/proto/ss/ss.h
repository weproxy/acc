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
R<proto::Handler, error> New(const string& servURL);

}  // namespace ss
}  // namespace proto
}  // namespace app
