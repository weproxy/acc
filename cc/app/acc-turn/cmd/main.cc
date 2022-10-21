//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/time/time.h"
#include "internal/core/core.h"
#include "logx/logx.h"

using namespace gx;

// #define _GXR_SET_V_(v) LOGS_D("_GXR_SET_V_")
// #define _GXR_SET_Vx(v) LOGS_D("_GXR_SET_Vx")

// #define GET_MACRO_(v, x1, x2) #v[0] == '_' ? x1 : x2
// #define GET_MACRO(v, x1, x2) GET_MACRO_(v, x1, x2)

// #define _SET_V(v) _GX_EXPAND(GET_MACRO(v, _GXR_SET_V_, _GXR_SET_Vx))(v)

// main ...
int main(int argc, char* argv[]) {
    LOGX_I("[main] ...");
    DEFER(LOGX_I("[main] exit"));

#if 0
    extern void unitest_run();
    {
        // LOGS_D("_GXR_SET_V(\"_\") =" << _SET_V(_));
        _SET_V(a_b);

        // unitest_run();
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
