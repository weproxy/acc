//
// weproxy@foxmail.com 2022/10/03
//

#include "proto.h"

#include "gx/net/url/url.h"
#include "logx/logx.h"

namespace app {
namespace proto {

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// _protos ...
using ProtoMap = Map<string, NewHandlerFn>;
static std::unique_ptr<ProtoMap> _protos;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Register ...
void Register(const string& proto, const NewHandlerFn& fn) {
    LOGS_D(TAG << " Register(" << proto << ")");
    if (!_protos) {
        _protos = std::unique_ptr<ProtoMap>(new ProtoMap);
    }
    (*_protos)[proto] = fn;
}

// error ...
static const error _errProtoEmptied = errors::New("emptied proto");
static const error _errProtoNotSupport = errors::New("unspport proto");

// GetHandler ...
R<Handler, error> GetHandler(const string& servURL) {
    if (!_protos) {
        return {nil, _errProtoEmptied};
    }

    AUTO_R(uri, err, url::Parse(servURL));
    if (err) {
        LOGS_D(TAG << " Parse(" << servURL << "), err: " << err);
        return {nil, err};
    }

    string proto = uri->Scheme;
    if (proto.empty()) {
        proto = "direct";
    }

    auto it = _protos->find(proto);
    if (it != _protos->end()) {
        return it->second(servURL);
    }

    return {nil, _errProtoNotSupport};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Init ..
error Init() {
    LOGS_D(TAG << " Init()");
    return nil;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Deinit ...
void Deinit() { LOGS_D(TAG << " Deinit()"); }

}  // namespace proto
}  // namespace app
