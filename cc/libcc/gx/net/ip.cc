//
// weproxy@foxmail.com 2022/10/03
//

#include "ip.h"

#include "gx/fmt/fmt.h"

namespace gx {
namespace net {

// v4InV6Prefix ...
static const uint8 v4InV6Prefix[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};

// IPv4 ...
IP IPv4(uint8 a, uint8 b, uint8 c, uint8 d) {
    IP p;
#if 0
    p.B = make(IPv6len);
    copy(p.B, v4InV6Prefix, sizeof(v4InV6Prefix));
    p.B[12] = a;
    p.B[13] = b;
    p.B[14] = c;
    p.B[15] = d;
#else
    p.B = make(IPv4len);
    p.B[0] = a;
    p.B[1] = b;
    p.B[2] = c;
    p.B[3] = d;
#endif
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

namespace xx {
// Bigger than we need, not too big to worry about overflow
const int big = 0xFFFFFF;

// Decimal to integer.
// Returns number, characters consumed, success.
R<int, int, bool> dtoi(const string& s) {
    int n = 0, i = 0;
    for (i = 0; i < len(s) && '0' <= s[i] && s[i] <= '9'; i++) {
        n = n * 10 + int(s[i] - '0');
        if (n >= big) {
            return {big, i, false};
        }
    }
    if (i == 0) {
        return {0, 0, false};
    }
    return {n, i, true};
}

// Hexadecimal to integer.
// Returns number, characters consumed, success.
R<int, int, bool> xtoi(const string& s) {
    int n = 0, i = 0;
    for (i = 0; i < len(s); i++) {
        if ('0' <= s[i] && s[i] <= '9') {
            n *= 16;
            n += int(s[i] - '0');
        } else if ('a' <= s[i] && s[i] <= 'f') {
            n *= 16;
            n += int(s[i] - 'a') + 10;
        } else if ('A' <= s[i] && s[i] <= 'F') {
            n *= 16;
            n += int(s[i] - 'A') + 10;
        } else {
            break;
        }
        if (n >= big) {
            return {0, i, false};
        }
    }
    if (i == 0) {
        return {0, i, false};
    }
    return {n, i, true};
}

// Parse IPv4 address (d.d.d.d).
IP parseIPv4(const string& src) {
    string s(src);
    bytez<> p = make(IPv4len);
    for (int i = 0; i < IPv4len; i++) {
        if (len(s) == 0) {
            // Missing octets.
            return nil;
        }
        if (i > 0) {
            if (s[0] != '.') {
                return nil;
            }
            s = s.substr(1);
        }
        AUTO_R(n, c, ok, dtoi(s));
        if (!ok || n > 0xFF) {
            return nil;
        }
        if (c > 1 && s[0] == '0') {
            // Reject non-zero components with leading zeroes.
            return nil;
        }
        s = s.substr(c);
        p[i] = byte(n);
    }
    if (len(s) != 0) {
        return nil;
    }
    return IPv4(p[0], p[1], p[2], p[3]);
}

// Index of rightmost occurrence of b in s.
int last(const string& s, byte b) {
    int i = len(s);
    for (i--; i >= 0; i--) {
        if (s[i] == b) {
            break;
        }
    }
    return i;
}

R<string, string> splitHostZone(const string& s) {
    // The IPv6 scoped addressing zone identifier starts after the
    // last percent sign.
    int i = last(s, '%');
    if (i > 0) {
        return {s.substr(0, i), s.substr(i + 1)};
    } else {
        return {s, ""};
    }
}

// parseIPv6 parses s as a literal IPv6 address described in RFC 4291
// and RFC 5952.
IP parseIPv6(const string& src) {
    string s(src);

    bytez<> ip = make(IPv6len);
    int ellipsis = -1;  // position of ellipsis in ip

    // Might have leading ellipsis
    if (len(s) >= 2 && s[0] == ':' && s[1] == ':') {
        ellipsis = 0;
        s = s.substr(2);
        // Might be only ellipsis
        if (len(s) == 0) {
            return ip;
        }
    }

    // Loop, parsing hex numbers followed by colon.
    int i = 0;
    while (i < IPv6len) {
        // Hex number.
        AUTO_R(n, c, ok, xtoi(s));
        if (!ok || n > 0xFFFF) {
            return nil;
        }

        // If followed by dot, might be in trailing IPv4.
        if (c < len(s) && s[c] == '.') {
            if (ellipsis < 0 && i != (IPv6len - IPv4len)) {
                // Not the right place.
                return nil;
            }
            if ((i + IPv4len) > IPv6len) {
                // Not enough room.
                return nil;
            }
            auto ip4 = parseIPv4(s);
            if (ip4 == nil) {
                return nil;
            }
            ip[i] = ip4[12];
            ip[i + 1] = ip4[13];
            ip[i + 2] = ip4[14];
            ip[i + 3] = ip4[15];
            s = "";
            i += IPv4len;
            break;
        }

        // Save this 16-bit chunk.
        ip[i] = byte(n >> 8);
        ip[i + 1] = byte(n);
        i += 2;

        // Stop at end of string.
        s = s.substr(c);
        if (len(s) == 0) {
            break;
        }

        // Otherwise must be followed by colon and more.
        if (s[0] != ':' || len(s) == 1) {
            return nil;
        }
        s = s.substr(1);

        // Look for ellipsis.
        if (s[0] == ':') {
            if (ellipsis >= 0) {  // already have one
                return nil;
            }
            ellipsis = i;
            s = s.substr(1);
            if (len(s) == 0) {  // can be at end
                break;
            }
        }
    }

    // Must have used entire string.
    if (len(s) != 0) {
        return nil;
    }

    // If didn't parse enough, expand ellipsis.
    if (i < IPv6len) {
        if (ellipsis < 0) {
            return nil;
        }
        int n = IPv6len - i;
        for (int j = i - 1; j >= ellipsis; j--) {
            ip[j + n] = ip[j];
        }
        for (int j = ellipsis + n - 1; j >= ellipsis; j--) {
            ip[j] = 0;
        }
    } else if (ellipsis >= 0) {
        // Ellipsis must represent at least one 0 group.
        return nil;
    }
    return ip;
}

// parseIPv6Zone parses s as a literal IPv6 address and its associated zone
// identifier which is described in RFC 4007.
R<IP, string> parseIPv6Zone(const string& s) {
    // s, zone := splitHostZone(s)
    string zone;
    return {parseIPv6(s), zone};
}

}  // namespace xx

// ParseIP ...
IP ParseIP(const string& s) {
    for (int i = 0; i < len(s); i++) {
        switch (s[i]) {
            case '.':
                return xx::parseIPv4(s);
            case ':':
                return xx::parseIPv6(s);
        }
    }
    return nil;
}

////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
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

    return len() > 0 ? "<nil>" : "<?>";
}

}  // namespace net
}  // namespace gx
