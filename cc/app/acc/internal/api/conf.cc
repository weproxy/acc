//
// weproxy@foxmail.com 2022/10/03
//

#include "api.h"
#include "gx/io/io.h"
#include "logx/logx.h"

namespace internal {
namespace api {

// confCli ...
struct confCli : public io::xx::closer_t {
    // Start ...
    error Start() {
        LOGF_D("%s confCli.Start()", TAG);
        return nil;
    }

    // Close ...
    error Close() {
        LOGF_D("%s confCli.Close()", TAG);
        return nil;
    }
};

// newConfCli ...
R<io::Closer, error> newConfCli() {
    auto m = NewRef<confCli>();
    auto err = m->Start();
    if (err != nil) {
        return {nil, err};
    }
    return {m, nil};
}

}  // namespace api
}  // namespace internal
