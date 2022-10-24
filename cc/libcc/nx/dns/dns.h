//
// weproxy@foxmail.com 2022/10/23
//

#pragma once

#include "gx/x/net/dns/dnsmessage/dnsmessage.h"

namespace nx {
namespace dns {
using namespace gx;
using namespace gx::dnsmessage;

extern const error ErrQueryNotFound;
extern const error ErrQueryDropped;
extern const error ErrQueryFaked;
extern const error ErrQueryHited;

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

// MakeAnswer
R<Ref<Message>, error> MakeAnswer(const bytez<> msg, slice<string> answerIPs);
R<Ref<Message>, error> MakeAnswer(const Message* msg, slice<string> answerIPs);

// MakeAnswerBytes ...
R<bytez<>, error> MakeAnswerBytes(const bytez<> msg, slice<string> answerIPs);
R<bytez<>, error> MakeAnswerBytes(const Message* msg, slice<string> answerIPs);

namespace xx {
// ToString ...
string ToString(const slice<Resource> arr);
}  // namespace xx

}  // namespace dns
}  // namespace nx

////////////////////////////////////////////////////////////////////////////////
namespace std {
// override ostream <<
inline ostream& operator<<(ostream& o, const gx::slice<gx::dnsmessage::Resource> v) {
    return o << nx::dns::xx::ToString(v);
}
}  // namespace std
