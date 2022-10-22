//
// weproxy@foxmail.com 2022/10/03
//

#include "proto.h"

#include "logx/logx.h"

namespace app {
namespace proto {

////////////////////////////////////////////////////////////////////////////////
//
// _protos ...
using ProtoMap = Map<string, NewServerFn>;
static Box<ProtoMap> _protos;

// _servers ...
static Map<string, Server> _servers;

////////////////////////////////////////////////////////////////////////////////
// Register ...
void Register(const string& proto, const NewServerFn& fn) {
    LOGS_D(TAG << " Register(" << proto << ")");
    if (!_protos) {
        _protos = NewBox<ProtoMap>();
    }
    (*_protos)[proto] = fn;
}

////////////////////////////////////////////////////////////////////////////////
// Init ..
error Init(const json::J& js) {
    LOGS_D(TAG << " Init()");

    if (!js.is_array()) {
        return errors::New("config empty");
    }

    for (auto& j : js) {
        if (!j.is_object() || j.find("proto") == j.end()) {
            continue;
        }

        auto proto = j["proto"];
        if (!proto.is_string() || proto.empty()) {
            continue;
        }

        if (j.find("disabled") != j.end()) {
            auto disabled = j["disabled"];
            if (disabled.is_boolean() && disabled) {
                continue;
            }
        }

        auto it = _protos->find(proto);
        if (it != _protos->end() && it->second) {
            AUTO_R(svr, err, it->second(j));
            if (err) {
                return err;
            }

            err = svr->Start();
            if (err) {
                return err;
            }

            _servers[proto] = svr;
        }
    }

    return nil;
}

////////////////////////////////////////////////////////////////////////////////
// Deinit ...
void Deinit() {
    for (auto it = _servers.rbegin(); it != _servers.rend(); it++) {
        it->second->Close();
    }
    _servers.clear();
    LOGS_D(TAG << " Deinit()");
}

}  // namespace proto
}  // namespace app
