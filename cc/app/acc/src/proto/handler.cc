//
// weproxy@foxmail.com 2022/10/03
//

#include "logx/logx.h"
#include "proto.h"
#include "rule/rule.h"

namespace app {
namespace proto {

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

// StackHandler ...
struct StackHandler : public IHandler {
    // Handle ...
    virtual error Handle(net::Conn c, net::Addr raddr) { return handleTCP(c, raddr); }

    // HandlePacket ...
    virtual error HandlePacket(net::PacketConn c, net::Addr raddr) { return handleUDP(c, raddr); };

    virtual void Close(){};
};

}  // namespace proto
}  // namespace app
