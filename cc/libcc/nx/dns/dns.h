//
// weproxy@foxmail.com 2022/10/23
//

#pragma once

#include "gx/x/net/dns/dnsmessage/dnsmessage.h"

namespace nx {
namespace dns {
using namespace gx;

// Message ...
using Message = dnsmessage::Message;

// Answer ...
struct Answer {
    string Name;
    bytez<> Data;
    Ref<Message> Msg;
};

// CacheOnRequest ...
R<Ref<Message>, Ref<Answer>, error> CacheOnRequest(bytez<> b);

// CacheOnResponse ...
R<Ref<Message>, error> CacheOnResponse(bytez<> b);

}  // namespace dns
}  // namespace nx
