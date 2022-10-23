//
// weproxy@foxmail.com 2022/10/23
//

#include "dns.h"
#include "logx/logx.h"

namespace nx {
namespace dns {

// CacheOnRequest ...
R<Ref<Message>, Ref<Answer>, error> CacheOnRequest(bytez<> b) {
    auto msg = NewRef<Message>();
    auto err = msg->Unpack(b);
    if (err) {
        LOGS_E("[dns] err: " << err);
        return {nil, nil, err};
    }
    LOGS_I("[dns] cache.Query: ");
    return {msg, nil, nil};
}

// CacheOnResponse ...
R<Ref<Message>, error> CacheOnResponse(bytez<> b) {
    auto msg = NewRef<Message>();
    auto err = msg->Unpack(b);
    if (err) {
        LOGS_E("[dns] err: " << err);
        return {nil, err};
    }
    LOGS_W("[dns] cache.Store: ");
    return {msg, nil};
}

}  // namespace dns
}  // namespace nx
