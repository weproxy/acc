//
// weproxy@foxmail.com 2022/10/21
//

#include "core.h"

#include "fx/signal/signal.h"
#include "gx/time/time.h"
#include "logx/logx.h"

namespace internal {
namespace core {

// Main ...
int Main(int argc, char* argv[]) {
    LOGS_W("press Ctrl+C to quit");

    // loop test
    gx::go([] {
        int64 i = 0;
        for (;;) {
            LOGS_D("hello " << i);
            time::Sleep(time::Second);
        }
    });

    // Wait for Ctrl+C or kill -x
    fx::signal::WaitNotify(
        [](int sig) {
            // got sig
            LOGS_W("[signal] got sig: " << sig);
        },
        SIGINT,   // ctrl + c
        SIGQUIT,  // kill -3
        SIGKILL,  // kill -9
        SIGTERM   // kill -15
    );

    return 0;
}

}  // namespace core
}  // namespace internal
