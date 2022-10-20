//
// weproxy@foxmail.com 2022/10/03
//

#include "socks.h"

#include "gx/errors/errors.h"
#include "logx/logx.h"

namespace nx {
namespace socks {

////////////////////////////////////////////////////////////////////////////////
// error ...
const error ErrUnrecognizedAddrType = errors::New("unrecognized address type");
const error ErrInvalidSocksVersion = errors::New("invalid socks version");
const error ErrUserAuthFailed = errors::New("user authentication failed");
const error ErrNoSupportedAuth = errors::New("no supported authentication mechanism");

const error ErrInvalidAddrType = errors::New("invalid address type");
const error ErrInvalidAddrLen = errors::New("invalid address length");

////////////////////////////////////////////////////////////////////////////////
// IP ...
net::IP Addr::IP() const {
    if (len(B) > 0) {
        switch (B[0]) {
            case AddrTypeIPv4:
                return net::IP(B(1, 1 + net::IPv4len));
            case AddrTypeIPv6:
                return net::IP(B(1, 1 + net::IPv6len));
            default:
                break;
        }
    }
    return net::IP{};
}

// Port ...
int Addr::Port() const {
    if (len(B) > 0) {
        switch (B[0]) {
            case AddrTypeIPv4:
                return int(B[1 + net::IPv4len]) << 8 | int(B[1 + net::IPv4len + 1]);
            case AddrTypeIPv6:
                return int(B[1 + net::IPv6len]) << 8 | int(B[1 + net::IPv6len + 1]);
            case AddrTypeDomain:
                return int(B[2 + B[1]]) << 8 | int(B[2 + B[1] + 1]);
            default:
                break;
        }
    }
    return 0;
}

// ToNetAddr ...
net::Addr Addr::ToNetAddr() const { return net::MakeAddr(IP(), Port()); }

// FromNetAddr ...
void Addr::FromNetAddr(net::Addr addr) {
    auto ip4 = addr->IP.To4();
    if (ip4) {
        B = make(1 + net::IPv4len + 2);
        B[0] = AddrTypeIPv4;
        copy(B(1, 1 + net::IPv4len), ip4.B);
        B[1 + net::IPv4len] = uint8(addr->Port >> 8);
        B[1 + net::IPv4len + 1] = uint8(addr->Port);
        return;
    }

    auto ip6 = addr->IP.To16();
    B = make(1 + net::IPv6len + 2);
    B[0] = AddrTypeIPv6;
    copy(B(1, 1 + net::IPv6len), ip6.B);
    B[1 + net::IPv6len] = uint8(addr->Port >> 8);
    B[1 + net::IPv6len + 1] = uint8(addr->Port);
}

// String ...
const string Addr::String() const {
    if (len(B) <= 0) {
        return "<nil>";
    }

    return GX_SS(IP() << ":" << Port());
}


////////////////////////////////////////////////////////////////////////////////
// ParseAddr ...
//	| ATYP | ADDR | PORT |
//	+------+------+------+
//	|  1   |  x   |  2   |
R<Ref<Addr>, error> ParseAddr(const bytez<> B) {
    if (len(B) < 1 + 1 + 1 + 2) {
        return {nil, ErrInvalidAddrLen};
    }

    int m = 0;
    if (AddrTypeIPv4 == B[0]) {
        m = 1 + net::IPv4len + 2;
    } else if (AddrTypeIPv6 == B[0]) {
        m = 1 + net::IPv6len + 2;
    } else if (AddrTypeDomain == B[0]) {
        m = 1 + 1 + B[1] + 2;  // DOMAIN_LEN = B[1]
    } else {
        return {nil, ErrInvalidAddrLen};
    }

    if (len(B) < m) {
        return {nil, ErrInvalidAddrLen};
    }
    return {NewRef<Addr>(B(0, m)), nil};
}

// CopyAddr ...
R<int, error> CopyAddr(bytez<> buf, Ref<Addr> addr) {
    if (addr || len(buf) >= len(addr->B)) {
        memcpy(buf.data(), addr->B.data(), len(addr->B));
        return {len(addr->B), nil};
    }
    return {0, errors::New("socks::WriteAddr fail")};
}

// AppendAddr ...
R<int, error> AppendAddr(bytez<> buf, Ref<Addr> addr) {
    if (addr) {
        for (int i = 0; i < len(addr->B); i++) {
            append(buf, addr->B[i]);
        }
        return {len(addr->B), nil};
    }
    return {0, errors::New("socks::AppendAddr fail")};
}

}  // namespace socks
}  // namespace nx
