//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/sync/sync.h"
#include "gx/time/time.h"
#include "internal/core/core.h"
#include "logx/logx.h"

// main ...
int main(int argc, char* argv[]) {
    LOGX_I("[main] ...");
    DEFER(LOGX_I("[main] exit"));

    int r;
    gx::sync::WaitGroup wg(1);
    gx::go([&]() {
        r = internal::core::Main(argc, argv);
        wg.Done();
    });
    wg.Wait();

    gx::time::Sleep(gx::time::Millisecond * 300);

    return r;
}
