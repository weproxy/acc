//
// weproxy@foxmail.com 2022/10/03
//

#include "qc.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"

namespace internal {
namespace proto {
namespace qc {

////////////////////////////////////////////////////////////////////////////////
// server_t ...
struct server_t : public proto::server_t {
    //  Start ...
    virtual error Start() override {
        LOGS_D(TAG << " Start()");
        return nil;
    }

    // Close ...
    virtual error Close() override {
        LOGS_D(TAG << " Close()");
        return nil;
    }
};

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j) {
    LOGS_V(TAG << " Conf: " << j);
    return {NewRef<server_t>(), nil};
}

////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    proto::Register({"qc", "quic"}, New);
    return true;
}();

}  // namespace qc
}  // namespace proto
}  // namespace internal
