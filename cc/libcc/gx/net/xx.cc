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
AddrInfoRet GetAddrInfo(const string& addr) {
    AUTO_R(host, port, err, SplitHostPort(addr.empty() ? "0.0.0.0:0" : addr));
    if (err) {
        return {{}, err};
    }

    return GetAddrInfo(host, (uint16)port);
}

// getFdAddr  ...
static Addr getFdAddr(SOCKET fd, bool peer) {
    if (fd <= 0) {
        return nil;
    }

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
Addr GetSockAddr(SOCKET fd) { return getFdAddr(fd, false); }

// GetPeerAddr ...
Addr GetPeerAddr(SOCKET fd) { return getFdAddr(fd, true); }

// CloseRead ...
void CloseRead(SOCKET fd) { co::shutdown(fd, 'r'); }

// CloseWrite ...
void CloseWrite(SOCKET fd) { co::shutdown(fd, 'w'); }

}  // namespace xx
}  // namespace net
}  // namespace gx
