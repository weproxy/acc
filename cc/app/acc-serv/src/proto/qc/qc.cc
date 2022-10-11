//
// weproxy@foxmail.com 2022/10/03
//

#include "qc.h"

#include "../proto.h"
#include "logx/logx.h"

namespace app {
namespace proto {
namespace qc {

////////////////////////////////////////////////////////////////////////////////////////////////////
// Server ...
struct Server : public proto::IServer {
    //  Start ...
    virtual error Start() override {
        LOGS_D(TAG << " Start()");
        return nil;
    }

    // Close ...
    virtual void Close() override { LOGS_D(TAG << " Close()"); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
static R<proto::Server, error> New(const json::J& j) {
    LOGS_V(TAG << " Conf: " << j);
    auto s = std::shared_ptr<Server>(new Server);
    return {s, nil};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    // LOGS_V(TAG << " init()");
    proto::Register("qc", New);
    return true;
}();

}  // namespace qc
}  // namespace proto
}  // namespace app
