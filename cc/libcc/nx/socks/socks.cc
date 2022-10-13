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
R<Addr, error> ParseAddr(const byte_s buf) {
    if (buf.size() < 1 + 1 + 1 + 2) {
        return {nil, ErrInvalidAddrLen};
    }

    switch (buf[0]) {
        case AddrTypeIPv4: {
            auto n = 1 + net::IPv4len + 2;
            if (buf.size() < n) {
                return {nil, ErrInvalidAddrLen};
            }
            return {Addr(new xx::addr_t(buf(0, n))), nil};
        }
        case AddrTypeIPv6: {
            auto n = 1 + net::IPv6len + 2;
            if (buf.size() < n) {
                return {nil, ErrInvalidAddrLen};
            }
            return {Addr(new xx::addr_t(buf(0, n))), nil};
        }
        case AddrTypeDomain: {
            auto n = 1 + 1 + buf[1] + 2;
            if (buf.size() < n) {
                return {nil, ErrInvalidAddrLen};
            }
            return {Addr(new xx::addr_t(buf(0, n))), nil};
        }
        default:
            return {nil, ErrInvalidAddrType};
    }
}

}  // namespace socks
}  // namespace nx
