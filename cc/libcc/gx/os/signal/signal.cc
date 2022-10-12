//
// weproxy@foxmail.com 2022/10/03
//

#include "signal.h"

#include "co/os.h"

namespace gx {
namespace signal {

namespace xx {

// _cbs ...
static Map<int, CallbackFn> _cbs;

// _wgs ...
static Map<int, std::shared_ptr<WaitGroup>> _wgs;

// sig_handler ..
static void sig_handler(int sig) {
    auto cb = _cbs.find(sig);
    if (cb != _cbs.end()) {
        if (cb->second) {
            cb->second(sig);
        }

        auto wg = _wgs.find(sig);
        if (wg != _wgs.end() && wg->second) {
            wg->second->Done();
        }
    }
}

// notify ...
void notify(const std::function<void(int)>& cb, int sig) {
    _cbs[sig] = cb;
    os::signal(sig, sig_handler);
}

// waitNotify ...
void waitNotify(std::shared_ptr<WaitGroup> wg, const std::function<void(int)>& cb, int sig) {
    _wgs[sig] = wg;
    notify(cb, sig);
}

}  // namespace xx
}  // namespace signal
}  // namespace gx
