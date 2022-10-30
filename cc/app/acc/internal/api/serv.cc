//
// weproxy@foxmail.com 2022/10/03
//

#include "api.h"
#include "gx/io/io.h"
#include "logx/logx.h"

namespace internal {
namespace api {

// ctrlServ ...
struct ctrlServ : public io::xx::closer_t {
    // Start ...
    error Start() {
        LOGF_D("%s ctrlServ.Start()", TAG);
        return nil;
    }

    // Close ...
    error Close() {
        LOGF_D("%s ctrlServ.Close()", TAG);
        return nil;
    }
};

// newCtrlServ ...
R<io::Closer, error> newCtrlServ() {
    auto m = NewRef<ctrlServ>();
    auto err = m->Start();
    if (err != nil) {
        return {nil, err};
    }
    return {m, nil};
}

}  // namespace api
}  // namespace internal
