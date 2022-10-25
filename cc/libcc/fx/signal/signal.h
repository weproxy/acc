//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <functional>

#include "gx/os/signal/signal.h"
#include "gx/sync/sync.h"

namespace fx {
namespace signal {
using namespace gx;

// CallbackFn ...
using CallbackFn = gx::signal::CallbackFn;

////////////////////////////////////////////////////////////////////////////////
namespace xx {

// waitNotify ...
void waitNotify(Ref<sync::WaitGroup> wg, const CallbackFn& cb, int sig);

// waitNotify ...
template <typename T, typename... Args>
void waitNotify(Ref<sync::WaitGroup> wg, const CallbackFn& cb, T&& sig, Args&&... args) {
    waitNotify(wg, cb, std::forward<T>(sig));
    waitNotify(wg, cb, std::forward<Args>(args)...);
}
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////
// WaitNotify ...
template <typename... T>
void WaitNotify(const CallbackFn& cb, T&&... sig) {
    auto wg = NewRef<sync::WaitGroup>(1);
    xx::waitNotify(wg, cb, std::forward<T>(sig)...);
    wg->Wait();
}

}  // namespace signal
}  // namespace fx
