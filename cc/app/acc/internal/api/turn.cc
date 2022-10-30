//
// weproxy@foxmail.com 2022/10/03
//

#include "api.h"
#include "gx/io/io.h"
#include "logx/logx.h"

namespace internal {
namespace api {

// turnCli ...
struct turnCli : public io::xx::closer_t {
    // Start ...
    error Start() {
        LOGF_D("%s turnCli.Start()", TAG);
        return nil;
    }

    // Close ...
    error Close() {
        LOGF_D("%s turnCli.Close()", TAG);
        return nil;
    }
};

// newTurnCli ...
R<io::Closer, error> newTurnCli() {
    auto m = NewRef<turnCli>();
    auto err = m->Start();
    if (err != nil) {
        return {nil, err};
    }
    return {m, nil};
}

}  // namespace api
}  // namespace internal
