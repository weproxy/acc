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

// GetAddrInfo ...
AddrInfoRet GetAddrInfo(const string& host, const string& port) {
    AddrInfo info(new addrInfo_t());
    int r = getaddrinfo(host.empty() ? "0.0.0.0" : host.c_str(), port.c_str(), NULL, &info->i);
    if (r < 0) {
        return {{}, errors::New("invalid address: %s", host.c_str())};
    }

    return {info, nil};
}

// GetAddrInfo ...
AddrInfoRet GetAddrInfo(const string& host, uint16 port) {
    return GetAddrInfo(host, string(str::from(port).c_str()));
}

// GetAddrInfo ...
AddrInfoRet GetAddrInfo(const string& addr) {
    AUTO_R(host, port, err, SplitHostPort(addr));
    if (err) {
        return {{}, err};
    }

    return GetAddrInfo(host, (uint16)port);
}

// _getSockAddr  ...
static Addr _getSockAddr(int fd, bool peer) {
    addr_in_t addr;
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
Addr GetSockAddr(int fd) { return _getSockAddr(fd, false); }

// GetPeerAddr ...
Addr GetPeerAddr(int fd) { return _getSockAddr(fd, true); }

}  // namespace xx
}  // namespace net
}  // namespace gx
