//
// weproxy@foxmail.com 2022/10/03
//

// #include "gx/net/url/url.h"
#include "gx/time/time.h"
#include "internal/core/core.h"
#include "logx/logx.h"

using namespace gx;

// main ...
int main(int argc, char* argv[]) {
    LOGX_I("[main] ...");
    DEFER(LOGX_I("[main] exit"));

    // if (true) {
    //     unitest::test_net_url();
    //     return 0;
    // }

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
