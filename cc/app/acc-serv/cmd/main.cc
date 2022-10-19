//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/time/time.h"
#include "internal/core/core.h"
#include "logx/logx.h"

using namespace gx;

// main ...
int main(int argc, char* argv[]) {
    LOGX_I("[main] ...");
    DEFER(LOGX_I("[main] exit"));

#if 0
    extern void unitest_run();
    {
        LOGF_D("a.a=%d, a.b=%d", a.a, a.b);

        unitest_run();
        return 0;
    }
#endif

    int r;
    WaitGroup wg(1);
    gx::go([&]() {
        r = app::core::Main(argc, argv);
        wg.Done();
    });
    wg.Wait();

    time::Sleep(time::Millisecond * 300);

    return r;
}
