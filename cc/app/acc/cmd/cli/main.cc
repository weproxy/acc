//
// weproxy@foxmail.com 2022/10/03
//

#include "core/core.h"
#include "logx/logx.h"
#include "gx/time/time.h"

using namespace gx;

// main ...
int main(int argc, char* argv[]) {
    LOGX_I("[main] ...");
    DEFER(LOGX_I("[main] exit"));

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
