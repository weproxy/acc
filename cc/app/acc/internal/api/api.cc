//
// weproxy@foxmail.com 2022/10/03
//

#include "api.h"

#include "gx/io/io.h"
#include "logx/logx.h"

namespace internal {
namespace api {

// extern
extern R<io::Closer, error> newCtrlServ();
extern R<io::Closer, error> newConfCli();
extern R<io::Closer, error> newTurnCli();

////////////////////////////////////////////////////////////////////////////////

// _closers ...
slice<io::Closer> _closers;

// closeAll ...
void closeAll() {
    for (int i = len(_closers) - 1; i >= 0; i--) {
        _closers[i]->Close();
    }
    _closers = nil;
}

// Init ...
error Init() {
    LOGF_D("%s Init()", TAG);

    ////////////////////////////////////////////////////////
    // api server
    AUTO_R(svr, er1, newCtrlServ());
    if (er1 != nil) {
        closeAll();
        return er1;
    }
    _closers = append(_closers, svr);

    ////////////////////////////////////////////////////////
    // conf client
    AUTO_R(cfg, er2, newConfCli());
    if (er2 != nil) {
        closeAll();
        return er2;
    }
    _closers = append(_closers, cfg);

    ////////////////////////////////////////////////////////
    // turn client
    AUTO_R(cli, er3, newTurnCli());
    if (er3 != nil) {
        closeAll();
        return er3;
    }
    _closers = append(_closers, cli);

    return nil;
}

// Deinit ...
error Deinit() {
    closeAll();
    LOGF_D("%s Deinit()", TAG);
    return nil;
}

}  // namespace api
}  // namespace internal
