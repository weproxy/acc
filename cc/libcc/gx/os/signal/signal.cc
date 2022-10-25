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

// sig_handler ..
static void sig_handler(int sig) {
    auto cb = _cbs.find(sig);
    if (cb != _cbs.end()) {
        if (cb->second) {
            cb->second(sig);
        }
    }
}

// notify ...
void notify(const CallbackFn& cb, int sig) {
    _cbs[sig] = cb;
    os::signal(sig, sig_handler);
}

}  // namespace xx
}  // namespace signal
}  // namespace gx
