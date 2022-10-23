//
// weproxy@foxmail.com 2022/10/23
//

#pragma once

#include "gx/x/net/dns/dnsmessage/dnsmessage.h"

namespace nx {
namespace dns {
using namespace gx;

extern const error ErrQueryNotFound;
extern const error ErrQueryDropped;
extern const error ErrQueryFaked;
extern const error ErrQueryHited;

// Message ...
using Message = dnsmessage::Message;

// Question ...
using Question = dnsmessage::Question;

// Answer ...
struct Answer {
    string Name;
    bytez<> Data;
    Ref<Message> Msg;
};

// OnRequest ...
R<Ref<Message>, Ref<Answer>, error> OnRequest(bytez<> b);

// OnResponse ...
R<Ref<Message>, error> OnResponse(bytez<> b);

}  // namespace dns
}  // namespace nx
