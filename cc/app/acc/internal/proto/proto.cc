//
// weproxy@foxmail.com 2022/10/03
//

#include "proto.h"

#include "gx/net/net.h"
#include "gx/net/url/url.h"
#include "logx/logx.h"
#include "nx/dns/dns.h"
#include "rule/rule.h"

namespace internal {
namespace proto {

////////////////////////////////////////////////////////////////////////////////
//
// _protos ...
using ProtoMap = Map<string, NewHandlerFn>;
static Box<ProtoMap> _protos;

////////////////////////////////////////////////////////////////////////////////
// Register ...
void Register(const stringz<>& protos, const NewHandlerFn& fn) {
    LOGS_D(TAG << " Register(" << protos << ")");
    if (!_protos) {
        _protos = NewBox<ProtoMap>();
    }
    for (auto& proto : protos) {
        (*_protos)[proto] = fn;
    }
}

// error ...
static const error _errProtoEmpty = errors::New("proto empty");
static const error _errProtoNotSupport = errors::New("this proto is not support");

// GetHandler ...
R<Handler, error> GetHandler(const string& servURL) {
    if (!_protos) {
        return {nil, _errProtoEmpty};
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

////////////////////////////////////////////////////////////////////////////////

// dnsSetFakeProvideFn ...
static error dnsSetFakeProvideFn() {
    // _fakes
    static auto _fakes = makemap<string /*domain*/, stringz<> /*ips*/>();

    // load fakes...
    auto err = []() -> error {
        // TODO...
        _fakes[string("fake.weproxy.test")] = {"1.2.3.4", "5.6.7.8"};
        return nil;
    }();
    if (err) {
        LOGS_E(TAG << " load dns fakes, err: " << err);
        return err;
    }

    // SetFakeProvideFn ...
    nx::dns::SetFakeProvideFn([](const string& domain, nx::dns::Type typ) -> stringz<> {
        AUTO_R(ss, ok, _fakes(domain));
        if (ok) {
            return ss;
        }
        return {};
    });

    return nil;
}

////////////////////////////////////////////////////////////////////////////////
// stackHandler_t ...
struct stackHandler_t : public handler_t {
    // Handle ...
    virtual void Handle(net::Conn c, net::Addr raddr) override {
        DEFER(c->Close());

        string servName;

        // HTTP/HTTPS
        if (raddr->Port == 80 || raddr->Port == 443) {
            // AUTO_R(host, err, sni::GetServerName(c));
        }

        AUTO_R(servURL, er1, rule::GetTCPRule(servName, raddr));
        if (er1) {
            LOGS_D(TAG << " Handle(), err: " << er1);
            return;
        }

        AUTO_R(h, er2, GetHandler(servURL));
        if (er2) {
            LOGS_D(TAG << " Handle(), err: " << er2);
            return;
        }

        h->Handle(c, raddr);
    }

    // HandlePacket ...
    virtual void HandlePacket(net::PacketConn pc, net::Addr raddr) override {
        DEFER(pc->Close());

        string servName;

        // DNS ..
        if (raddr->Port == 53) {
            // AUTO_R(host, err, dns::GetServerName(c));
        }

        AUTO_R(servURL, er1, rule::GetUDPRule(servName, raddr));
        if (er1) {
            LOGS_D(TAG << " HandlePacket(), err: " << er1);
            return;
        }

        AUTO_R(h, er2, GetHandler(servURL));
        if (er2) {
            LOGS_D(TAG << " HandlePacket(), err: " << er2);
            return;
        }

        h->HandlePacket(pc, raddr);
    }

    // Close ...
    virtual error Close() override { return nil; }
};

////////////////////////////////////////////////////////////////////////////////
// Init ..
error Init() {
    LOGS_D(TAG << " Init()");

    // dns SetFakeProvideFn ...
    dnsSetFakeProvideFn();

    // stack SetHandler
    nx::stack::SetHandler(NewRef<stackHandler_t>());

    return nil;
}

////////////////////////////////////////////////////////////////////////////////
// Deinit ...
error Deinit() {
    LOGS_D(TAG << " Deinit()");
    return nil;
}

}  // namespace proto
}  // namespace internal
