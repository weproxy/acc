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
constexpr int MaxAddrLen = 1 + 1 + 255 + 2;

////////////////////////////////////////////////////////////////////////////////
// error ...
extern const error ErrInvalidAddrType;
extern const error ErrInvalidAddrLen;

////////////////////////////////////////////////////////////////////////////////
// AddrType  ...
enum class AddrType : uint8 {
    IPv4 = 1,
    Domain = 3,
    IPv6 = 4,
};

// ToString ...
inline const char* ToString(const AddrType e) {
#define CASE_RETURN_ADDRTYPE(e) \
    case AddrType::e:           \
        return "Addr" #e

    switch (e) {
        CASE_RETURN_ADDRTYPE(IPv4);
        CASE_RETURN_ADDRTYPE(Domain);
        CASE_RETURN_ADDRTYPE(IPv6);
        default:
            return "";
    }
}

////////////////////////////////////////////////////////////////////////////////
// Addr ...
struct Addr {
    bytez<> B;

    Addr() = default;
    Addr(const bytez<> b) : B(b) {}

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

////////////////////////////////////////////////////////////////////////////////
// ToNetAddr ...
inline net::Addr ToNetAddr(Ref<Addr> addr) { return addr->ToNetAddr(); }

// FromNetAddr ...
inline Ref<Addr> FromNetAddr(net::Addr addr) {
    auto r = NewRef<Addr>();
    r->FromNetAddr(addr);
    return r;
}

////////////////////////////////////////////////////////////////////////////////
// ReadAddr ..
//     | ATYP | ADDR | PORT |
//     +------+------+------+
//     |  1   |  x   |  2   |
template <typename Reader, typename std::enable_if<io::xx::has_read<Reader>::value, int>::type = 0>
R<int /*readlen*/, Ref<Addr>, error> ReadAddr(Reader r) {
    bytez<> B = make(MaxAddrLen);

    // 2bytes = ATYP + (MAYBE)DOMAIN_LEN
    AUTO_R(n, err, io::ReadFull(r, B(0, 2)));
    if (err) {
        return {n, nil, err};
    }

    int m = 0;
    if (AddrType::IPv4 == AddrType(B[0])) {
        m = 1 + net::IPv4len + 2;
    } else if (AddrType::IPv6 == AddrType(B[0])) {
        m = 1 + net::IPv6len + 2;
    } else if (AddrType::Domain == AddrType(B[0])) {
        m = 1 + 1 + B[1] + 2;  // DOMAIN_LEN = B[1]
    } else {
        return {n, nil, ErrInvalidAddrLen};
    }

    AUTO_R(t, er2, io::ReadFull(r, B(2, m)));
    n += t > 0 ? t : 0;
    if (er2) {
        return {n, nil, er2};
    }

    return {n, NewRef<Addr>(B(0, m)), nil};
}

////////////////////////////////////////////////////////////////////////////////
// ParseAddr ...
R<Ref<Addr>, error> ParseAddr(const bytez<> buf);

}  // namespace socks
}  // namespace nx

////////////////////////////////////////////////////////////////////////////////
namespace std {
// override ostream <<
inline ostream& operator<<(ostream& o, const nx::socks::AddrType v) { return o << nx::socks::ToString(v); }
}  // namespace std
