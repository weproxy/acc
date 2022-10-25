//
// weproxy@foxmail.com 2022/10/23
//

#include "dns.h"

#include "gx/fmt/fmt.h"
#include "gx/net/net.h"
#include "gx/strings/strings.h"
#include "logx/logx.h"

namespace nx {
namespace dns {

// ...
const error ErrQueryNotFound = errors::New("not found");
const error ErrQueryDropped = errors::New("dropped");
const error ErrQueryFaked = errors::New("faked");
const error ErrQueryHited = errors::New("hited");

namespace xx {
// ToString ...
string ToString(slice<Resource> arr) {
    stringz<> answs;

    for (int i = 0; i < len(arr); i++) {
        auto ans = arr[i];
        if (ans.Body == nil) {
            continue;
        }
        auto* p = ans.Body.get();
        switch (ans.Header.Type) {
            case Type::A:
                answs = append(answs, net::IP(((AResource*)p)->A).String());
                break;
            case Type::AAAA:
                answs = append(answs, net::IP(((AAAAResource*)p)->AAAA).String());
                break;
            case Type::CNAME:
                answs = append(answs, ((CNAMEResource*)p)->CNAME.String());
                break;
            // case Type::NS:
            //     answs = append(answs, ((NSResource*)p)->NS.String());
            default:
                answs = append(answs, fmt::Sprintf("Type%v", int(ans.Header.Type)));
                break;
        }
    }

    return GX_SS(answs);
}
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////

// cacheLoad ...
extern R<Ref<Answer>, error> cacheLoad(Ref<Message> msg);

// cacheStore ...
extern error cacheStore(Ref<Message> msg);

////////////////////////////////////////////////////////////////////////////////

// Name ...
string Answer::Name() const {
    if (Msg && len(Msg->Questions) > 0) {
        return strings::TrimRight(Msg->Questions[0].Name.String(), ".");
    }
    return "";
}

// Type ...
Type Answer::Type() const {
    if (Msg && len(Msg->Questions) > 0) {
        return Msg->Questions[0].Type;
    }
    return Type::A;
}

// Bytes ...
bytez<> Answer::Bytes() const {
    if (Msg) {
        AUTO_R(b, err, Msg->Pack());
        if (err == nil) {
            return b;
        }
    }
    return {};
}

////////////////////////////////////////////////////////////////////////////////

// OnRequest ...
R<Ref<Message>, Ref<Answer>, error> OnRequest(bytez<> b) {
    auto msg = NewRef<Message>();
    auto err = msg->Unpack(b);
    if (err) {
        LOGS_E("[dns] err: " << err);
        return {nil, nil, err};
    }

    AUTO_R(answ, _, cacheLoad(msg));

    return {msg, answ, nil};
}

// OnResponse ...
R<Ref<Message>, error> OnResponse(bytez<> b) {
    auto msg = NewRef<Message>();
    auto err = msg->Unpack(b);
    if (err) {
        LOGS_E("[dns] err: " << err);
        return {nil, err};
    }

    err = cacheStore(msg);

    return {msg, nil};
}

}  // namespace dns
}  // namespace nx
