//
// weproxy@foxmail.com 2022/10/03
//

#include "core.h"

#include "co/os.h"
#include "conf/conf.h"
#include "logx/logx.h"
#include "gx/os/signal/signal.h"
#include "proto/proto.h"
// #include "server/server.h"

namespace app {
namespace core {

// Main ...
int Main(int argc, char* argv[]) {
    // LOGX_I("[app] cpunum =", os::cpunum());

    // flag::init(argc, argv);

    AUTO_R(js, err, conf::ReadConfig());
    if (err) {
        LOGS_E("[core] conf::ReadConfig(), err: " << err);
        return -1;
    }

    // proto init
    err = proto::Init(js["server"]);
    if (err) {
        LOGS_E("[core] proto::Init(), err: " << err);
        proto::Deinit();
        return -1;
    }
    // proto deinit
    DEFER(proto::Deinit());

    // Wait Ctrl+C or kill -x
    signal::WaitNotify(
        [](int sig) {
            LOGS_W("[signal] got sig = " << sig);
        },
        SIGINT /*ctrl+c*/, SIGQUIT /*kill -3*/, SIGKILL /*kill -9*/, SIGTERM /*kill -15*/);

    return 0;
}

}  // namespace core
}  // namespace app
