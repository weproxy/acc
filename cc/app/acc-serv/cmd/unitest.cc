//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/net/url/url.h"
#include "logx/logx.h"

using namespace gx;

// unitest_run ...
void unitest_run() {
    LOGX_I("[unitest] ...");
    DEFER(LOGX_I("[unitest] exit"));

#if 1
    unitest::test_net_url();
#endif
}
