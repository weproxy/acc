//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/time/time.h"

namespace gx {
namespace net {
namespace xx {

// timeoutMs ...
int timeoutMs(const time::Time& t1, const time::Time& t2);

// addrInfo_t ...
struct addrInfo_t {
    struct addrinfo* i;

    addrInfo_t() : i(0) {}
    ~addrInfo_t() {
        if (i) {
            freeaddrinfo(i);
        }
    }

    const struct addrinfo* operator->() { return i; }
    const struct addrinfo* operator->() const { return i; }
};

// addr_in_t ...
typedef union {
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
} addr_in_t;

// ToAddr ...
inline Addr ToAddr(const addr_in_t& addr) {
    IP ip;
    uint16 port;
    if (addr.v4.sin_family == AF_INET) {
        ip.B = make(IPv4len);
        copy(ip.B, &addr.v4.sin_addr.s_addr, IPv4len);
        port = ntoh16(addr.v4.sin_port);
    } else if (addr.v6.sin6_family == AF_INET6) {
        ip.B = make(IPv6len);
        copy(ip.B, &addr.v6.sin6_addr.__u6_addr, IPv6len);
        port = ntoh16(addr.v6.sin6_port);
    }
    return MakeAddr(ip, port);
}

// FromAddr ...
inline R<addr_in_t, int> FromAddr(Addr addr) {
    addr_in_t ain;
    memset(&ain, 0, sizeof(ain));

    auto ip4 = addr->IP.To4();
    if (ip4) {
        ain.v4.sin_family = AF_INET;
        memcpy(&ain.v4.sin_addr.s_addr, ip4.B.data(), IPv4len);
        ain.v4.sin_port = hton16(addr->Port);
        return {ain, sizeof(ain.v4)};
    } else {
        auto ip6 = addr->IP.To16();
        ain.v4.sin_family = AF_INET6;
        memcpy(&ain.v6.sin6_addr.__u6_addr, ip6.B.data(), IPv6len);
        ain.v6.sin6_port = hton16(addr->Port);
        return {ain, sizeof(ain.v6)};
    }
}

// AddrInfo
typedef std::shared_ptr<addrInfo_t> AddrInfo;

// GetAddrInfo ...
typedef R<AddrInfo, error> AddrInfoRet;
AddrInfoRet GetAddrInfo(const string& host, const string& port);
inline AddrInfoRet GetAddrInfo(const string& host, uint16 port) {return GetAddrInfo(host, GX_SS(port));}
AddrInfoRet GetAddrInfo(const string& addr);

// GetSockAddr ...
Addr GetSockAddr(SOCKET fd);

// GetPeerAddr ...
Addr GetPeerAddr(SOCKET fd);

// CloseRead ...
void CloseRead(SOCKET fd);

// CloseWrite ...
void CloseWrite(SOCKET fd);

}  // namespace xx
}  // namespace net
}  // namespace gx
