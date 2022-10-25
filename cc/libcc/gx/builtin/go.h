//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "co/co.h"

////////////////////////////////////////////////////////////////////////////////
//
namespace gx {

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
