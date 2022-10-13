//
// weproxy@foxmail.com 2022/10/03
//

#include "ip.h"

#include "fmt/fmt.h"

namespace gx {
namespace net {

// v4InV6Prefix ...
static const uint8 v4InV6Prefix[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};

// IPv4 ...
IP IPv4(uint8 a, uint8 b, uint8 c, uint8 d) {
    IP p;
    p.B = make(IPv4len);
    copy(p.B, v4InV6Prefix, sizeof(v4InV6Prefix));
    p.B[12] = a;
    p.B[13] = b;
    p.B[14] = c;
    p.B[15] = d;
    return p;
}

// makeIPv6 ...
static IP makeIPv6(uint8 a0, uint8 a1, uint8 a14, uint8 a15) {
    IP p;
    p.B = make(IPv6len);
    memset(p.B.data(), 0, p.len());
    p.B[0] = a0;
    p.B[1] = a1;
    p.B[14] = a14;
    p.B[15] = a15;
    return p;
}

// IPv4Mask returns the IP mask (in 4-byte form) of the
// IPv4 mask a.b.c.d.
IPMask IPv4Mask(uint8 a, uint8 b, uint8 c, uint8 d) {
    IPMask p;
    p.B = make(IPv4len);
    p.B[0] = a;
    p.B[1] = b;
    p.B[2] = c;
    p.B[3] = d;
    return p;
}

// ParseIP ...
IP ParseIP(const string& s) { return {}; }

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Well-known IPv4 addresses
const IP IPv4bcast = IPv4(255, 255, 255, 255);  // limited broadcast
const IP IPv4allsys = IPv4(224, 0, 0, 1);       // all systems
const IP IPv4allrouter = IPv4(224, 0, 0, 2);    // all routers
const IP IPv4zero = IPv4(0, 0, 0, 0);           // all zeros

// Well-known IPv6 addresses
const IP IPv6zero = makeIPv6(0, 0, 0, 0);
const IP IPv6unspecified = makeIPv6(0, 0, 0, 0);
const IP IPv6loopback = makeIPv6(0, 0, 0, 1);
const IP IPv6interfacelocalallnodes = makeIPv6(0xff, 0x01, 0, 0x01);
const IP IPv6linklocalallnodes = makeIPv6(0xff, 0x02, 0, 0x01);
const IP IPv6linklocalallrouters = makeIPv6(0xff, 0x02, 0, 0x02);

////////////////////////////////////////////////////////////////////////////////////////////////////
//

// Equal ...
bool IP::Equal(const IP& x) const {
    if (&x == this) {
        return true;
    }
    if (len() == x.len()) {
        return memcmp(B.data(), x.B.data(), len()) == 0;
    }
    if (len() == IPv4len && x.len() == IPv6len) {
        return memcmp(x.B.data(), v4InV6Prefix, 12) == 0 && memcmp(B.data(), x.B.data() + 12, 4) == 0;
    }
    if (len() == IPv6len && x.len() == IPv4len) {
        return memcmp(B.data(), v4InV6Prefix, 12) == 0 && memcmp(x.B.data(), B.data() + 12, 4) == 0;
    }
    return false;
}

// IsUnspecified ...
bool IP::IsUnspecified() const { return Equal(IPv4zero) || Equal(IPv6unspecified); }

// IsLoopback ...
bool IP::IsLoopback() const {
    IP ip4 = To4();
    if (ip4) {
        return ip4.B[0] == 127;
    }
    return Equal(IPv6loopback);
}

// IsPrivate ...
bool IP::IsPrivate() const {
    IP ip4 = To4();
    if (ip4) {
        // Following RFC 1918, Section 3. Private Address Space which says:
        //   The Internet Assigned Numbers Authority (IANA) has reserved the
        //   following three blocks of the IP address space for private internets:
        //     10.0.0.0        -   10.255.255.255  (10/8 prefix)
        //     172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
        //     192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
        return ip4.B[0] == 10 || (ip4.B[0] == 172 && (ip4.B[1] & 0xf0) == 16) || (ip4.B[0] == 192 && ip4.B[1] == 168);
    }
    // Following RFC 4193, Section 8. IANA Considerations which says:
    //   The IANA has assigned the FC00::/7 prefix to "Unique Local Unicast".
    return len() == IPv6len && (B[0] & 0xfe) == 0xfc;
}

// IsMulticast ...
bool IP::IsMulticast() const {
    IP ip4 = To4();
    if (ip4) {
        return (ip4.B[0] & 0xf0) == 0xff;
    }
    return len() == IPv6len && B[0] == 0xff;
}

// IsInterfaceLocalMulticast ...
bool IP::IsInterfaceLocalMulticast() const { return len() == IPv6len && B[0] == 0xff && (B[1] & 0x0f) == 0x01; }

// IsLinkLocalMulticast ...
bool IP::IsLinkLocalMulticast() const {
    IP ip4 = To4();
    if (ip4) {
        return ip4.B[0] == 224 && ip4.B[1] == 0 && ip4.B[2] == 0;
    }
    return len() == IPv6len && B[0] == 0xff && (B[1] & 0xf) == 0x02;
}

// IsLinkLocalUnicast ...
bool IP::IsLinkLocalUnicast() const {
    IP ip4 = To4();
    if (ip4) {
        return ip4.B[0] == 169 && ip4.B[1] == 254;
    }
    return len() == IPv6len && B[0] == 0xfe && (B[1] & 0xc0) == 0x80;
}

// IsGlobalUnicast ...
bool IP::IsGlobalUnicast() const {
    return (len() == IPv4len || len() == IPv6len) && !Equal(IPv4bcast) && !IsUnspecified() && !IsLoopback() &&
           !IsMulticast() && !IsLinkLocalUnicast();
}

// Is p all zeros?
bool isZeros(const uint8 b[], uint8 len) {
    for (uint8 i = 0; i < len; i++) {
        if (b[i] != 0) {
            return false;
        }
    }
    return true;
}

// To4 ...
IP IP::To4() const {
    if (len() == IPv4len) {
        return *this;
    }
    if (len() == IPv6len && isZeros(B, 10) && B[10] == 0xff && B[11] == 0xff) {
        return IPv4(B[12], B[13], B[14], B[15]);
    }
    return {};
}

// To16 ...
IP IP::To16() const {
    if (len() == IPv4len) {
        return IPv4(B[0], B[1], B[2], B[3]);
    }
    if (len() == IPv6len) {
        return *this;
    }
    return {};
}

// String ...
string IP::String() const {
    if (len() == IPv4len) {
        return fmt::Sprintf("%d.%d.%d.%d", B[0], B[1], B[2], B[3]);
    } else if (len() == IPv6len) {
        // TODO...
    }

    return len() > 0  ? "<nil>"  :"<?>";
}

}  // namespace net
}  // namespace gx
