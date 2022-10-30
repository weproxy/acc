//
// weproxy@foxmail.com 2022/10/03
//

#include "core.h"

#include "fx/signal/signal.h"
#include "internal/api/api.h"
#include "internal/board/board.h"
#include "internal/conf/conf.h"
#include "internal/iptbl/iptbl.h"
#include "internal/proto/proto.h"
#include "logx/logx.h"

namespace internal {
namespace core {

// Main ...
int Main(int argc, char* argv[]) {
    flag::init(argc, argv);

    if (true) {
        board::Detect();
        iptbl::Detect();
    }

    // proto init
    auto err = proto::Init();
    if (err) {
        LOGS_E("[core] proto::Init(), err: " << err);
        proto::Deinit();
        return -1;
    }
    // proto deinit
    DEFER(proto::Deinit());

    // api init
    err = api::Init();
    if (err) {
        LOGS_E("[core] api::Init(), err: " << err);
        api::Deinit();
        return -1;
    }
    // api deinit
    DEFER(api::Deinit());

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
