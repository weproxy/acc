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
// testDefer ...
inline void testDefer() {
    Defer defer;

    DEFER(std::cout << "A exit" << std::endl);
    defer([] { std::cout << "A0" << std::endl; });
    defer([] { std::cout << "A1" << std::endl; });

    {
        DEFER(std::cout << "B exit" << std::endl);
        defer([] { std::cout << "B0" << std::endl; });
        defer([] { std::cout << "B1" << std::endl; });
    }

    for (int i = 0; i < 2; i++) {
        DEFER(std::cout << "C" << i << " exit" << std::endl);
        defer([i]() { std::cout << "C" << i << std::endl; });
    }
}
}  // namespace unitest
}  // namespace gx
