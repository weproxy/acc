//
// weproxy@foxmail.com 2022/10/23
//

#include "dns.h"
#include "gx/net/net.h"
#include "gx/strings/strings.h"

namespace nx {
namespace dns {

// _FakeProvideFn ...
static FakeProvideFn _FakeProvideFn;

// SetFakeProvideFn ...
void SetFakeProvideFn(const FakeProvideFn& fn) { _FakeProvideFn = fn; }

// MakeFakeAnswer ...
// if provideFn is nil, use global fn set by SetFakeProvideFn()
R<Ref<Answer>, error> MakeFakeAnswer(const bytez<> msg, const FakeProvideFn& provideFn) {
    Message m;
    auto err = m.Unpack(msg);
    if (err) {
        return {nil, err};
    }
    return MakeFakeAnswer(&m, provideFn);
}

// MakeFakeAnswer ...
// if provideFn is nil, use global fn set by SetFakeProvideFn()
R<Ref<Answer>, error> MakeFakeAnswer(const Message* msg, const FakeProvideFn& provideFn) {
    if ((!provideFn && !_FakeProvideFn) || !msg || len(msg->Questions) == 0) {
        return {nil, errors::New("invalid param")};
    }

    auto& q = msg->Questions[0];

    auto name = strings::TrimSuffix(q.Name.String(), ".");
    auto ansIPs = provideFn ? provideFn(name, q.Type) : _FakeProvideFn(name, q.Type);
    if (len(ansIPs) == 0) {
        return {nil, errors::New("no answer IPs")};
    }

    auto newMsg = NewRef<Message>();

    *newMsg = *msg;
    newMsg->Authorities = nil;
    newMsg->Additionals = nil;
    newMsg->Header.Response = true;

    for (int i = 0; i < len(ansIPs); i++) {
        auto ip = net::ParseIP(ansIPs[i]);
        if (!ip) {
            continue;
        }

        Resource res;
        res.Header.Class = Class::INET;
        res.Header.Type = q.Type;
        res.Header.Name = q.Name;
        res.Header.TTL = 30;

        switch (q.Type) {
            case Type::A: {
                auto ip4 = ip.To4();
                if (ip4) {
                    auto r = NewRef<AResource>();
                    copy(r->A, ip4.B);
                    res.Body = r;
                    newMsg->Answers = append(newMsg->Answers, res);
                }
                break;
            }
            case Type::AAAA: {
                auto ip6 = ip.To16();
                if (ip6) {
                    auto r = NewRef<AAAAResource>();
                    copy(r->AAAA, ip6.B);
                    res.Body = r;
                    newMsg->Answers = append(newMsg->Answers, res);
                }
                break;
            }
            default:
                break;
        }
    }

    if (len(newMsg->Answers) == 0) {
        return {nil, errors::New("no answer IPs")};
    }

    auto answer = NewRef<Answer>();
    answer->Msg = newMsg;

    // println("len(newMsg->Answers) =", len(newMsg->Answers));

    return {answer, nil};
}

}  // namespace dns
}  // namespace nx
