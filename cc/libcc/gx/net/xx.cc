//
// weproxy@foxmail.com 2022/10/03
//

#include "co/str.h"
#include "net.h"

namespace gx {
namespace net {
namespace xx {

// timeoutMs ...
int timeoutMs(const time::Time& t1, const time::Time& t2) {
    auto deadline = [&]() {
        if (t1 && t2) {
            return t1 < t2 ? t1 : t2;
        }
        return t1 ? t1 : t2;
    }();

    if (deadline) {
        auto d = time::Since(deadline);
        if (d < 0) {
            return -d.Milliseconds();
        }
    }

    return -1;
}

// ToAddr ...
Addr ToAddr(const AddrIn& addr) {
    IP ip;
    uint16 port = 0;
    if (addr.v4.sin_family == AF_INET) {
        byte* p = (byte*)&addr.v4.sin_addr.s_addr;
        ip = IP(bytez<>{p[0], p[1], p[2], p[3]});
        port = ntoh16(addr.v4.sin_port);
    } else if (addr.v6.sin6_family == AF_INET6) {
        ip.B = make(IPv6len);
        copy(ip.B, &addr.v6.sin6_addr.__u6_addr, IPv6len);
        port = ntoh16(addr.v6.sin6_port);
    }
    return MakeAddr(ip, port);
}

// FromAddr ...
R<AddrIn, int> FromAddr(Addr addr) {
    AddrIn ain;
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

// GetAddrInfo ...
R<Ref<AddrInfo>, error> GetAddrInfo(const string& host, const string& port) {
    auto info = NewRef<AddrInfo>();
    int r = getaddrinfo(host.empty() ? "0.0.0.0" : host.c_str(), port.c_str(), NULL, &info->i);
    if (r < 0 || !info->i) {
        return {nil, fmt::Errorf("invalid address: %s", host.c_str())};
    }

    return {info, nil};
}

// GetAddrInfo ...
R<Ref<AddrInfo>, error> GetAddrInfo(const string& addr) {
    AUTO_R(host, port, err, SplitHostPort(addr.empty() ? "0.0.0.0:0" : addr));
    if (err) {
        return {nil, err};
    }

    return GetAddrInfo(host, (uint16)port);
}

// getFdAddr  ...
static Addr getFdAddr(SOCKET fd, bool peer) {
    if (fd <= 0) {
        return nil;
    }

    AddrIn addr;
    socklen_t addrLen = sizeof(addr);

    int r;
    if (peer) {
        r = ::getpeername(fd, (sockaddr*)&addr, &addrLen);
    } else {
        r = ::getsockname(fd, (sockaddr*)&addr, &addrLen);
    }
    if (r != 0) {
        return nil;
    }

    return ToAddr(addr);
}

// GetSockAddr ...
Addr GetSockAddr(SOCKET fd) { return getFdAddr(fd, false); }

// GetPeerAddr ...
Addr GetPeerAddr(SOCKET fd) { return getFdAddr(fd, true); }

// CloseRead ...
error CloseRead(SOCKET fd) { return 0 == co::shutdown(fd, 'r') ? nil : errors::New(co::strerror()); }

// CloseWrite ...
error CloseWrite(SOCKET fd) { return 0 == co::shutdown(fd, 'w') ? nil : errors::New(co::strerror()); }

}  // namespace xx
}  // namespace net
}  // namespace gx
