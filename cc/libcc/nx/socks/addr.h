//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/io/io.h"
#include "gx/net/net.h"

namespace nx {
namespace socks {
using namespace gx;

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
    AddrTypeDomain = 3,
    AddrTypeIPv6 = 4,
};

////////////////////////////////////////////////////////////////////////////////
namespace xx {
struct addr_t {
    slice<byte> B;

    addr_t() = default;
    addr_t(const slice<byte> b) : B(b) {}

    operator bool() const { return (bool)B; }
    uint8 operator[](size_t i) const { return B[i]; }
    uint8& operator[](size_t i) { return B[i]; }

    byte* data() { return B.data(); }
    const byte* data() const { return B.data(); }
    int size() { return len(B); }
    const int size() const { return len(B); }

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
//     | ATYP | ADDR | PORT |
//     +------+------+------+
//     |  1   |  x   |  2   |
template <typename Reader, typename std::enable_if<io::xx::has_read<Reader>::value, int>::type = 0>
R<size_t /*readlen*/, Addr, error> ReadAddr(Reader r) {
    slice<byte> B = make(MaxAddrLen);

    // 2bytes = ATYP + (MAYBE)DOMAIN_LEN
    AUTO_R(n, err, io::ReadFull(r, B(0, 2)));
    if (err) {
        return {n, nil, err};
    }

    int m = 0;
    if (AddrTypeIPv4 == B[0]) {
        m = 1 + net::IPv4len + 2;
    } else if (AddrTypeIPv6 == B[0]) {
        m = 1 + net::IPv6len + 2;
    } else if (AddrTypeDomain == B[0]) {
        m = 1 + 1 + B[1] + 2;  // DOMAIN_LEN = B[1]
    } else {
        return {n, nil, ErrInvalidAddrLen};
    }

    AUTO_R(t, er2, io::ReadFull(r, B(2, m)));
    n += t > 0 ? t : 0;
    if (er2) {
        return {n, nil, er2};
    }

    return {n, Addr(new xx::addr_t(B(0, m))), nil};
}

////////////////////////////////////////////////////////////////////////////////
// ParseAddr ...
R<Addr, error> ParseAddr(const slice<byte> buf);

// CopyAddr ...
R<int, error> CopyAddr(slice<byte> buf, Addr addr);

// AppendAddr ...
R<int, error> AppendAddr(slice<byte> buf, Addr addr);

}  // namespace socks
}  // namespace nx
