//
// weproxy@foxmail.com 2022/10/03
//

#include "core.h"

#include "fx/signal/signal.h"
#include "internal/conf/conf.h"
#include "internal/proto/proto.h"
#include "logx/logx.h"

namespace internal {
namespace core {

// Main ...
int Main(int argc, char* argv[]) {
    AUTO_R(js, err, conf::ReadConfig());
    if (err) {
        LOGS_E("[core] conf::ReadConfig(), err: " << err);
        return -1;
    }

    // proto init
    err = proto::Init(js["server"]);
    // proto deinit
    DEFER(proto::Deinit());
    if (err) {
        LOGS_E("[core] proto::Init(), err: " << err);
        return -1;
    }

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
