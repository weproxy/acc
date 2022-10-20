//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "co/co.h"

////////////////////////////////////////////////////////////////////////////////
//
namespace gx {

// WaitGroup ...
// using WaitGroup = co::WaitGroup;
class WaitGroup {
   public:
    // initialize the counter as @n
    explicit WaitGroup(uint32 n) : wg_(n){};

    // the counter is 0 by default
    WaitGroup() : wg_(0) {}

    WaitGroup(WaitGroup&& wg) : wg_(wg.wg_) {}

    // copy constructor, allow WaitGroup to be captured by value in lambda.
    WaitGroup(const WaitGroup& wg) : wg_(wg.wg_) {}

    void operator=(const WaitGroup&) = delete;

    // increase WaitGroup counter by n (1 by default)
    void Add(uint32 n = 1) const { wg_.add(n); }

    // decrease WaitGroup counter by 1
    void Done() const { wg_.done(); }

    // blocks until the counter becomes 0
    void Wait() const { wg_.wait(); }

   private:
    co::WaitGroup wg_;
};

// go ...
inline void go(co::Closure* cb) { co::go(cb); }

// go ...
template <typename F>
inline void go(F&& f) {
    co::go(co::new_closure(std::forward<F>(f)));
}

// go ...
template <typename F, typename P>
inline void go(F&& f, P&& p) {
    co::go(co::new_closure(std::forward<F>(f), std::forward<P>(p)));
}

// go ...
template <typename F, typename T, typename P>
inline void go(F&& f, T* t, P&& p) {
    co::go(co::new_closure(std::forward<F>(f), t, std::forward<P>(p)));
}

}  // namespace gx
