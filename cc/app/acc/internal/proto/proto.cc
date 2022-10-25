//
// weproxy@foxmail.com 2022/10/03
//

#include "proto.h"

#include "gx/net/net.h"
#include "gx/net/url/url.h"
#include "logx/logx.h"
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
void Register(const string& proto, const NewHandlerFn& fn) {
    LOGS_D(TAG << " Register(" << proto << ")");
    if (!_protos) {
        _protos = NewBox<ProtoMap>();
    }
    (*_protos)[proto] = fn;
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
// handleTCP ...
static error handleTCP(net::Conn c, net::Addr raddr) {
    string servName;

    // HTTP/HTTPS
    if (raddr->Port == 80 || raddr->Port == 443) {
        // AUTO_R(host, err, sni::GetServerName(c));
    }

    AUTO_R(servURL, er1, rule::GetTCPRule(servName, raddr));
    if (er1) {
        return er1;
    }

    AUTO_R(h, er2, GetHandler(servURL));
    if (er2) {
        return er2;
    }

    return h->Handle(c, raddr);
}

// handleUDP ...
static error handleUDP(net::PacketConn c, net::Addr raddr) {
    string servName;

    // DNS ..
    if (raddr->Port == 53) {
        // AUTO_R(host, err, dns::GetServerName(c));
    }

    AUTO_R(servURL, er1, rule::GetUDPRule(servName, raddr));
    if (er1) {
        return er1;
    }

    AUTO_R(h, er2, GetHandler(servURL));
    if (er2) {
        return er2;
    }

    return h->HandlePacket(c, raddr);
}

// stackHandler_t ...
struct stackHandler_t : public handler_t {
    // Handle ...
    virtual error Handle(net::Conn c, net::Addr raddr) override { return handleTCP(c, raddr); }

    // HandlePacket ...
    virtual error HandlePacket(net::PacketConn c, net::Addr raddr) override { return handleUDP(c, raddr); }

    virtual error Close() override { return nil; }
};

////////////////////////////////////////////////////////////////////////////////
// Init ..
error Init() {
    LOGS_D(TAG << " Init()");
    nx::stack::SetHandler(NewRef<stackHandler_t>());
    return nil;
}

////////////////////////////////////////////////////////////////////////////////
// Deinit ...
void Deinit() { LOGS_D(TAG << " Deinit()"); }

}  // namespace proto
}  // namespace internal
