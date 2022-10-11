//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/io/io.h"
#include "gx/net/net.h"

namespace nx {
namespace socks {

////////////////////////////////////////////////////////////////////////////////
// MaxAddrLen ...
const int MaxAddrLen = 1 + 1 + 255 + 2;

////////////////////////////////////////////////////////////////////////////////
// error ...
extern const error ErrInvalidAddrType;
extern const error ErrInvalidAddrLen;

////////////////////////////////////////////////////////////////////////////////
// AddrType  ...
enum AddrType {
    AddrTypeIPv4 = 1,
    AddrTypeDomain = 2,
    AddrTypeIPv6 = 3,
};

////////////////////////////////////////////////////////////////////////////////
namespace xx {
struct addr_t {
    uint8 B[MaxAddrLen];
    size_t L;

    addr_t() : L(0) {}
    addr_t(const void* b, size_t l) : L(l) { memcpy(B, b, l); }

    operator bool() const { return L > 0; }
    uint8 operator[](size_t i) const { return B[i]; }
    uint8& operator[](size_t i) { return B[i]; }

    // IP ...
    net::IP IP() const;

    // Port ...
    int Port() const;

    // IPPort ...
    R<net::IP, int> IPPort() const { return {IP(), Port()}; }

    // ToNetAddr ...
    net::Addr ToNetAddr() const;

    // FromNetAddr ...
    void FromNetAddr(net::Addr addr);

    // String ...
    const string String() const;
};
}  // namespace xx

// Addr ...
typedef std::shared_ptr<xx::addr_t> Addr;

////////////////////////////////////////////////////////////////////////////////
// ToNetAddr ...
inline net::Addr ToNetAddr(Addr addr) { return addr->ToNetAddr(); }

// FromNetAddr ...
inline Addr FromNetAddr(net::Addr addr) {
    Addr r(new xx::addr_t());
    r->FromNetAddr(addr);
    return r;
}

////////////////////////////////////////////////////////////////////////////////
// ReadAddr ..
template <typename Reader, typename std::enable_if<gx::io::xx::is_reader<Reader>::value, int>::type = 0>
R<size_t /*readlen*/, Addr, error> ReadAddr(Reader r) {
    Addr addr(new xx::addr_t());

    AUTO_R(n, err, io::ReadFull(r, addr->B, 2));
    if (err) {
        return {n, nil, err};
    }
    addr->L += n;

    switch (addr->B[0]) {
        case AddrTypeIPv4: {
            auto n = 1 + net::IPv4len;
            AUTO_R(t, err, io::ReadFull(r, addr->B + 2, n));
            addr->L += t > 0 ? t : 0;
            if (err) {
                return {addr->L, nil, err};
            }
            break;
        }
        case AddrTypeIPv6: {
            auto n = 1 + net::IPv6len;
            AUTO_R(t, err, io::ReadFull(r, addr->B + 2, n));
            addr->L += t > 0 ? t : 0;
            if (err) {
                return {addr->L, nil, err};
            }
            break;
        }
        case AddrTypeDomain: {
            auto n = 1 + 1 + addr->B[1];
            AUTO_R(t, err, io::ReadFull(r, addr->B + 2, n));
            addr->L += t > 0 ? t : 0;
            if (err) {
                return {addr->L, nil, err};
            }
            break;
        }
        default:
            return {addr->L, nil, ErrInvalidAddrLen};
    }

    return {addr->L, addr, nil};
}

////////////////////////////////////////////////////////////////////////////////
// ParseAddr ...
R<Addr, error> ParseAddr(void* data, size_t len);

}  // namespace socks
}  // namespace nx
