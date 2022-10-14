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
    AddrTypeDomain = 2,
    AddrTypeIPv6 = 3,
};

////////////////////////////////////////////////////////////////////////////////
namespace xx {
struct addr_t {
    byte_s B;

    addr_t() = default;
    addr_t(const byte_s b) : B(b) {}

    operator bool() const { return (bool)B; }
    uint8 operator[](size_t i) const { return B[i]; }
    uint8& operator[](size_t i) { return B[i]; }

    byte* data() { return B.data(); }
    const byte* data() const { return B.data(); }
    int size() { return B.size(); }
    const int size() const { return B.size(); }

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
template <typename Reader, typename std::enable_if<io::xx::is_reader<Reader>::value, int>::type = 0>
R<size_t /*readlen*/, Addr, error> ReadAddr(Reader r) {
    byte_s B = make(256);

    AUTO_R(n, err, io::ReadFull(r, B(0, 2)));
    if (err) {
        return {n, nil, err};
    }
    int L = n;

    switch (B[0]) {
        case AddrTypeIPv4: {
            auto n = 1 + net::IPv4len;
            AUTO_R(t, err, io::ReadFull(r, B(2, 2 + n)));
            L += t > 0 ? t : 0;
            if (err) {
                return {L, nil, err};
            }
            break;
        }
        case AddrTypeIPv6: {
            auto n = 1 + net::IPv6len;
            AUTO_R(t, err, io::ReadFull(r, B(2, 2 + n)));
            L += t > 0 ? t : 0;
            if (err) {
                return {L, nil, err};
            }
            break;
        }
        case AddrTypeDomain: {
            auto n = 1 + 1 + B[1];
            AUTO_R(t, err, io::ReadFull(r, B(2, 2 + n)));
            L += t > 0 ? t : 0;
            if (err) {
                return {L, nil, err};
            }
            break;
        }
        default:
            return {L, nil, ErrInvalidAddrLen};
    }

    return {L, Addr(new xx::addr_t(B(0, L))), nil};
}

////////////////////////////////////////////////////////////////////////////////
// ParseAddr ...
R<Addr, error> ParseAddr(const byte_s buf);

// CopyAddr ...
R<int, error> CopyAddr(byte_s buf, Addr addr);

// AppendAddr ...
R<int, error> AppendAddr(byte_s buf, Addr addr);

}  // namespace socks
}  // namespace nx
