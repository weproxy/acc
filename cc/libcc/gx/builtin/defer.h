//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <functional>
#include <iostream>
#include <list>

#ifdef defer
#undef defer
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
// Defer ...
struct Defer final {
    Defer() = default;
    Defer(const std::function<void()>& fn) { defer(fn); }

    ~Defer() {
        for (auto& f : l_) {
            f();
        }
    }

    Defer& operator()(const std::function<void()>& fn) {
        defer(fn);
        return *this;
    }

    void defer(const std::function<void()>& fn) { l_.push_front(fn); }

    std::list<std::function<void()>> l_;
};

#define _GX_DEFER_CAT(x, n) x##n
#define _GX_DEFER_NAME(x, n) _GX_DEFER_CAT(x, n)
}  // namespace gx

// DEFER ...
#define DEFER(e) gx::Defer _GX_DEFER_NAME(_gx_defer_, __LINE__)([&]() { e; })

// DEFER_ADD ...
#define DEFER_ADD(d, e) d([&]() { e; })

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
namespace unitest {
void test_defer();
}  // namespace unitest
}  // namespace gx
