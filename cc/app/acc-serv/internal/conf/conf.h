//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"
#include "gx/encoding/json/json.h"

namespace internal {
namespace conf {

// Iface ...
struct Iface {
    string in;
    string out;
};

// Conf ...
struct Conf {
    Iface iface;
    Vec<string> server;
};

// ReadConfig ...
R<json::J, error> ReadConfig();

}  // namespace conf
}  // namespace internal
