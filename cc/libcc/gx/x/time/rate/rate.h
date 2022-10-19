//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/time/time.h"

namespace gx {
namespace rate {

// Reservation ...
struct Reservation {
    bool OK() { return false; }
    int Delay() { return 0; }
};

// Limiter ...
struct Limiter {
    Reservation ReserveN(const time::Time& now, int n);
};

}  // namespace rate
}  // namespace gx
