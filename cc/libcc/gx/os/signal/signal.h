//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <functional>

#include "gx/gx.h"

namespace gx {
namespace signal {

////////////////////////////////////////////////////////////////////////////////////////////////////
// Callback ...
typedef std::function<void(int)> CallbackFn;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace xx {
// notify ...
void notify(const CallbackFn& cb, int sig);

// notify ...
template <typename T, typename... Args>
void notify(const CallbackFn& cb, T&& sig, Args&&... args) {
    notify(cb, std::forward<T>(sig));
    notify(cb, std::forward<Args>(args)...);
}

// waitNotify ...
void waitNotify(SharedPtr<WaitGroup> wg, const CallbackFn& cb, int sig);

// waitNotify ...
template <typename T, typename... Args>
void waitNotify(SharedPtr<WaitGroup> wg, const CallbackFn& cb, T&& sig, Args&&... args) {
    waitNotify(wg, cb, std::forward<T>(sig));
    waitNotify(wg, cb, std::forward<Args>(args)...);
}
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
// Notify ...
template <typename... T>
void Notify(const CallbackFn& cb, T&&... sig) {
    xx::notify(cb, std::forward<T>(sig)...);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// WaitNotify ...
template <typename... T>
void WaitNotify(const CallbackFn& cb, T&&... sig) {
    auto wg = SharedPtr<WaitGroup>(new WaitGroup(1));
    xx::waitNotify(wg, cb, std::forward<T>(sig)...);
    wg->Wait();
}

}  // namespace signal
}  // namespace gx
