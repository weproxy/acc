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
namespace xx {
// IP ...
net::IP addr_t::IP() const {
    if (B.size() > 0) {
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
int addr_t::Port() const {
    if (B.size() > 0) {
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
net::Addr addr_t::ToNetAddr() const { return net::MakeAddr(IP(), Port()); }

// FromNetAddr ...
void addr_t::FromNetAddr(net::Addr addr) {
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
const string addr_t::String() const {
    if (B.size() <= 0) {
        return "<nil>";
    }

    return GX_SS(IP() << ":" << Port());
}

}  // namespace xx

////////////////////////////////////////////////////////////////////////////////
// ParseAddr ...
//	| ATYP | ADDR | PORT |
//	+------+------+------+
//	|  1   |  x   |  2   |
R<Addr, error> ParseAddr(const slice<byte> B) {
    if (B.size() < 1 + 1 + 1 + 2) {
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

    if (B.size() < m) {
        return {nil, ErrInvalidAddrLen};
    }
    return {Addr(new xx::addr_t(B(0, m))), nil};
}

// CopyAddr ...
R<int, error> CopyAddr(slice<byte> buf, Addr addr) {
    if (addr || buf.size() >= addr->B.size()) {
        memcpy(buf.data(), addr->B.data(), addr->B.size());
        return {addr->B.size(), nil};
    }
    return {0, errors::New("socks::WriteAddr fail")};
}

// AppendAddr ...
R<int, error> AppendAddr(slice<byte> buf, Addr addr) {
    if (addr) {
        for (int i = 0; i < addr->B.size(); i++) {
            append(buf, addr->B[i]);
        }
        return {addr->B.size(), nil};
    }
    return {0, errors::New("socks::AppendAddr fail")};
}

}  // namespace socks
}  // namespace nx
