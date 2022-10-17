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

// limiter_t ...
struct limiter_t {
    Reservation ReserveN(const time::Time& now, int n);
};

// Limiter ...
typedef SharedPtr<limiter_t> Limiter;

}  // namespace rate
}  // namespace gx
