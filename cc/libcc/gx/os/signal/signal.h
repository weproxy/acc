//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <functional>

#include "gx/gx.h"

namespace gx {
namespace signal {

////////////////////////////////////////////////////////////////////////////////
// Callback ...
using CallbackFn = func<void(int)>;

////////////////////////////////////////////////////////////////////////////////
namespace xx {
// notify ...
void notify(const CallbackFn& cb, int sig);

// notify ...
template <typename T, typename... Args>
void notify(const CallbackFn& cb, T&& sig, Args&&... args) {
    notify(cb, std::forward<T>(sig));
    notify(cb, std::forward<Args>(args)...);
}
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////
// Notify ...
template <typename... T>
void Notify(const CallbackFn& cb, T&&... sig) {
    xx::notify(cb, std::forward<T>(sig)...);
}

}  // namespace signal
}  // namespace gx
