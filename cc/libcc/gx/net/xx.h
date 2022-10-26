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

// AddrInfo ...
struct AddrInfo {
    struct addrinfo* i{0};

    ~AddrInfo() {
        if (i) {
            freeaddrinfo(i);
        }
    }

    const struct addrinfo* operator->() { return i; }
    const struct addrinfo* operator->() const { return i; }
};

// AddrIn ...
typedef union {
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
} AddrIn;

// ToAddr ...
Addr ToAddr(const AddrIn& addr);

// FromAddr ...
R<AddrIn, int> FromAddr(Addr addr);

// GetAddrInfo ...
R<Ref<AddrInfo>, error> GetAddrInfo(const string& host, const string& port);
R<Ref<AddrInfo>, error> GetAddrInfo(const string& addr);
inline R<Ref<AddrInfo>, error> GetAddrInfo(const string& host, uint16 port) { return GetAddrInfo(host, GX_SS(port)); }

// GetSockAddr ...
Addr GetSockAddr(SOCKET fd);

// GetPeerAddr ...
Addr GetPeerAddr(SOCKET fd);

// CloseRead ...
error CloseRead(SOCKET fd);

// CloseWrite ...
error CloseWrite(SOCKET fd);

}  // namespace xx
}  // namespace net
}  // namespace gx
