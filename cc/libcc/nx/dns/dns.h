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

////////////////////////////////////////////////////////////////////////////////

// Answer ...
struct Answer {
    Ref<Message> Msg;

    string Name() const;
    Type Type() const;
    bytez<> Bytes() const;
};

// OnRequest ...
R<Ref<Message>, Ref<Answer>, error> OnRequest(bytez<> b);

// OnResponse ...
R<Ref<Message>, error> OnResponse(bytez<> b);

////////////////////////////////////////////////////////////////////////////////

// FakeProvideFn ...
using FakeProvideFn = func<stringz<>/*ips*/(const string&/*domain*/, Type/*typ*/)>;

// SetFakeProvideFn ...
void SetFakeProvideFn(const FakeProvideFn& fn);

// MakeFakeAnswer
R<Ref<Answer>, error> MakeFakeAnswer(const bytez<> msg, const FakeProvideFn& provideFn);
R<Ref<Answer>, error> MakeFakeAnswer(const Message* msg, const FakeProvideFn& provideFn);

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
