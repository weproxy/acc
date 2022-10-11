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
    if (L > 0) {
        switch (B[0]) {
            case AddrTypeIPv4:
                return net::IP(B + 1, net::IPv4len);
            case AddrTypeIPv6:
                return net::IP(B + 1, net::IPv6len);
            default:
                break;
        }
    }
    return net::IP{};
}

// Port ...
int addr_t::Port() const {
    if (L > 0) {
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
net::Addr addr_t::ToNetAddr() const {
    net::Addr r(new net::xx::addr_t());
    r->IP = IP();
    r->Port = Port();
    return r;
}

// FromNetAddr ...
void addr_t::FromNetAddr(net::Addr addr) {
    auto r = this;
    auto ip4 = addr->IP.To4();
    if (ip4) {
        r->B[0] = AddrTypeIPv4;
        memcpy(r->B + 1, ip4.B, net::IPv4len);
        r->B[1 + net::IPv4len] = uint8(addr->Port >> 8);
        r->B[1 + net::IPv4len + 1] = uint8(addr->Port);
        r->L = 1 + net::IPv4len;
        return;
    }

    auto ip6 = addr->IP.To16();
    r->B[0] = AddrTypeIPv6;
    memcpy(r->B + 1, ip6.B, net::IPv6len);
    r->B[1 + net::IPv6len] = uint8(addr->Port >> 8);
    r->B[1 + net::IPv6len + 1] = uint8(addr->Port);
    r->L = 1 + net::IPv6len;
}

// String ...
const string addr_t::String() const {
    if (L <= 0) {
        return "<nil>";
    }

    return GX_SS(IP() << ":" << Port());
}

}  // namespace xx

////////////////////////////////////////////////////////////////////////////////
// ParseAddr ...
R<Addr, error> ParseAddr(void* data, size_t len) {
    if (len < 1 + 1 + 1 + 2) {
        return {nil, ErrInvalidAddrLen};
    }

    uint8* buf = (uint8*)data;

    switch (buf[0]) {
        case AddrTypeIPv4: {
            auto n = 1 + net::IPv4len + 2;
            if (len < n) {
                return {nil, ErrInvalidAddrLen};
            }
            return {Addr(new xx::addr_t(buf, n)), nil};
        }
        case AddrTypeIPv6: {
            auto n = 1 + net::IPv6len + 2;
            if (len < n) {
                return {nil, ErrInvalidAddrLen};
            }
            return {Addr(new xx::addr_t(buf, n)), nil};
        }
        case AddrTypeDomain: {
            auto n = 1 + 1 + buf[1] + 2;
            if (len < n) {
                return {nil, ErrInvalidAddrLen};
            }
            return {Addr(new xx::addr_t(buf, n)), nil};
        }
        default:
            return {nil, ErrInvalidAddrType};
    }
}

}  // namespace socks
}  // namespace nx
