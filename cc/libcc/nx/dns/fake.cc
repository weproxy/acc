//
// weproxy@foxmail.com 2022/10/23
//

#include "dns.h"
#include "gx/net/net.h"

namespace nx {
namespace dns {

// MakeAnswer ...
R<Ref<Message>, error> MakeAnswer(const bytez<> msg, slice<string> answerIPs) {
    Message m;
    auto err = m.Unpack(msg);
    if (err) {
        return {nil, err};
    }
    return MakeAnswer(&m, answerIPs);
}

// MakeAnswer ...
R<Ref<Message>, error> MakeAnswer(const Message* msg, slice<string> answerIPs) {
    if (!msg || len(msg->Questions) == 0) {
        return {nil, errors::New("invalid msg")};
    }

    auto& q = msg->Questions[0];

    auto newMsg = NewRef<Message>();

    *newMsg = *msg;
    newMsg->Authorities = nil;
    newMsg->Additionals = nil;
    newMsg->Header.Response = true;

    for (int i = 0; i < len(answerIPs); i++) {
        auto ip = net::ParseIP(answerIPs[i]);
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

    // println("len(newMsg->Answers) =", len(newMsg->Answers));

    return {newMsg, nil};
}

// MakeAnswerBytes ...
R<bytez<>, error> MakeAnswerBytes(const bytez<> msg, slice<string> answerIPs) {
    AUTO_R(newMsg, err, MakeAnswer(msg, answerIPs));
    if (err) {
        return {nil, err};
    }
    return newMsg->Pack();
}

// MakeAnswerBytes ...
R<bytez<>, error> MakeAnswerBytes(const Message* msg, slice<string> answerIPs) {
    AUTO_R(newMsg, err, MakeAnswer(msg, answerIPs));
    if (err) {
        return {nil, err};
    }
    return newMsg->Pack();
}

}  // namespace dns
}  // namespace nx
