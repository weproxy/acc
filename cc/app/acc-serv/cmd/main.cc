//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/sync/sync.h"
#include "gx/time/time.h"
#include "internal/core/core.h"
#include "logx/logx.h"

using namespace gx;
using namespace internal;

// main ...
int main(int argc, char* argv[]) {
    LOGX_I("[main] ...");
    DEFER(LOGX_I("[main] exit"));

    bytez<> b{'a', 'b', 'c'};
    for (auto& c : b) {
       LOGS_D("c =" << c);
    }

    int r;
    sync::WaitGroup wg(1);
    gx::go([&]() {
        r = core::Main(argc, argv);
        wg.Done();
    });
    wg.Wait();

    time::Sleep(time::Millisecond * 300);

    return r;
}
