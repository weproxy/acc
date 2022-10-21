//
// weproxy@foxmail.com 2022/10/21
//

#include "core.h"

#include "gx/os/signal/signal.h"
#include "gx/time/time.h"
#include "logx/logx.h"

namespace app {
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
    signal::WaitNotify([](int sig) { LOGS_W("[signal] got sig = " << sig); }, SIGINT /*ctrl+c*/, SIGQUIT /*kill -3*/,
                       SIGKILL /*kill -9*/, SIGTERM /*kill -15*/);

    return 0;
}

}  // namespace core
}  // namespace app
