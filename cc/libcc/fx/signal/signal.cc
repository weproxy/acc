//
// weproxy@foxmail.com 2022/10/03
//

#include "signal.h"

namespace fx {
namespace signal {

namespace xx {

// sigcb ...
struct sigcb {
    Ref<sync::WaitGroup> wg;
    CallbackFn cb;

    sigcb() = default;
    sigcb(Ref<sync::WaitGroup> w, const CallbackFn& c) : wg(w), cb(c) {}
};

// _wait ...
static Map<int, sigcb> _wait;

// sig_handler ..
static void sig_handler(int sig) {
    auto it = _wait.find(sig);
    if (it != _wait.end()) {
        it->second.cb(sig);
        it->second.wg->Done();
    }
}

// waitNotify ...
void waitNotify(Ref<sync::WaitGroup> wg, const CallbackFn& cb, int sig) {
    _wait[sig] = sigcb(wg, cb);
    gx::signal::Notify(sig_handler, sig);
}

}  // namespace xx
}  // namespace signal
}  // namespace fx
