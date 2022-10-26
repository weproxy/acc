//
// weproxy@foxmail.com 2022/10/03
//

#include "ip.h"

#include "gx/bytes/bytes.h"
#include "gx/fmt/fmt.h"
#include "gx/strings/strings.h"

namespace gx {
namespace net {

// v4InV6Prefix ...
static const bytez<> v4InV6Prefix{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};

// IPv4 ...
IP IPv4(uint8 a, uint8 b, uint8 c, uint8 d) {
    IP p;
#if 1
    p.B = make(IPv6len);
    copy(p.B, v4InV6Prefix);
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
namespace xx {
IP makeIPv6(uint8 a0, uint8 a1, uint8 a14, uint8 a15) {
    IP p;
    p.B = make(IPv6len);
    memset(p.B.data(), 0, IPv6len);
    p.B[0] = a0;
    p.B[1] = a1;
    p.B[14] = a14;
    p.B[15] = a15;
    return p;
}
} // namespace xx

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

// CIDRMask returns an IPMask consisting of 'ones' 1 bits
// followed by 0s up to a total length of 'bits' bits.
// For a mask of this form, CIDRMask is the inverse of IPMask.Size.
IPMask CIDRMask(int ones, int bits) {
    if (bits != (8 * IPv4len) && bits != (8 * IPv6len)) {
        return {};
    }
    if (ones < 0 || ones > bits) {
        return {};
    }
    int l = bits / 8;
    bytez<> m = make(l);
    int n = uint(ones);
    for (int i = 0; i < l; i++) {
        if (n >= 8) {
            m[i] = 0xff;
            n -= 8;
            continue;
        }
        m[i] = ~byte(0xff >> n);
        n = 0;
    }
    IPMask r;
    r.B = m;
    return r;
}

namespace xx {
// Bigger than we need, not too big to worry about overflow
static const int big = 0xFFFFFF;

// Decimal to integer.
// Returns number, characters consumed, success.
static R<int, int, bool> dtoi(const string& s) {
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
static R<int, int, bool> xtoi(const string& s) {
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
static IP parseIPv4(const string& src) {
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
static int last(const string& s, byte b) {
    int i = len(s);
    for (i--; i >= 0; i--) {
        if (s[i] == b) {
            break;
        }
    }
    return i;
}

static R<string, string> splitHostZone(const string& s) {
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
static IP parseIPv6(const string& src) {
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

// ParseIP parses s as an IP address, returning the result.
// The string s can be in IPv4 dotted decimal ("192.0.2.1"), IPv6
// ("2001:db8::68"), or IPv4-mapped IPv6 ("::ffff:192.0.2.1") form.
// If s is not a valid textual representation of an IP address,
// ParseIP returns nil.
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
const IP IPv6zero = xx::makeIPv6(0, 0, 0, 0);
const IP IPv6unspecified = xx::makeIPv6(0, 0, 0, 0);
const IP IPv6loopback = xx::makeIPv6(0, 0, 0, 1);
const IP IPv6interfacelocalallnodes = xx::makeIPv6(0xff, 0x01, 0, 0x01);
const IP IPv6linklocalallnodes = xx::makeIPv6(0xff, 0x02, 0, 0x01);
const IP IPv6linklocalallrouters = xx::makeIPv6(0xff, 0x02, 0, 0x02);

////////////////////////////////////////////////////////////////////////////////
//

// Equal ...
bool IP::Equal(const IP& x) const {
    if (&x == this) {
        return true;
    }
    if (len(B) == len(x.B)) {
        return memcmp(B.data(), x.B.data(), len(B)) == 0;
    }
    if (len(B) == IPv4len && len(x.B) == IPv6len) {
        return memcmp(x.B.data(), v4InV6Prefix, 12) == 0 && memcmp(B.data(), x.B.data() + 12, 4) == 0;
    }
    if (len(B) == IPv6len && len(x.B) == IPv4len) {
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
        return ip4[0] == 10 || (ip4[0] == 172 && (ip4[1] & 0xf0) == 16) || (ip4[0] == 192 && ip4[1] == 168);
    }
    // Following RFC 4193, Section 8. IANA Considerations which says:
    //   The IANA has assigned the FC00::/7 prefix to "Unique Local Unicast".
    return len(B) == IPv6len && (B[0] & 0xfe) == 0xfc;
}

// IsMulticast ...
bool IP::IsMulticast() const {
    IP ip4 = To4();
    if (ip4) {
        return (ip4[0] & 0xf0) == 0xff;
    }
    return len(B) == IPv6len && B[0] == 0xff;
}

// IsInterfaceLocalMulticast ...
bool IP::IsInterfaceLocalMulticast() const { return len(B) == IPv6len && B[0] == 0xff && (B[1] & 0x0f) == 0x01; }

// IsLinkLocalMulticast ...
bool IP::IsLinkLocalMulticast() const {
    IP ip4 = To4();
    if (ip4) {
        return ip4[0] == 224 && ip4[1] == 0 && ip4[2] == 0;
    }
    return len(B) == IPv6len && B[0] == 0xff && (B[1] & 0xf) == 0x02;
}

// IsLinkLocalUnicast ...
bool IP::IsLinkLocalUnicast() const {
    IP ip4 = To4();
    if (ip4) {
        return ip4[0] == 169 && ip4[1] == 254;
    }
    return len(B) == IPv6len && B[0] == 0xfe && (B[1] & 0xc0) == 0x80;
}

// IsGlobalUnicast ...
bool IP::IsGlobalUnicast() const {
    return (len(B) == IPv4len || len(B) == IPv6len) && !Equal(IPv4bcast) && !IsUnspecified() && !IsLoopback() &&
           !IsMulticast() && !IsLinkLocalUnicast();
}

namespace xx {
// Is p all zeros?
static bool isZeros(const bytez<>& p) {
    for (int i = 0; i < len(p); i++) {
        if (p[i] != 0) {
            return false;
        }
    }
    return true;
}
}  // namespace xx

// To4 converts the IPv4 address ip to a 4-byte representation.
// If ip is not an IPv4 address, To4 returns nil.
IP IP::To4() const {
    if (len(B) == IPv4len) {
        return *this;
    }
    if (len(B) == IPv6len && xx::isZeros(B(0,10)) && B[10] == 0xff && B[11] == 0xff) {
        return IP(B(12, 16));
    }
    return {};
}

// DefaultMask returns the default IP mask for the IP address ip.
// Only IPv4 addresses have default masks; DefaultMask returns
// nil if ip is not a valid IPv4 address.
IP IP::To16() const {
    if (len(B) == IPv4len) {
        return IPv4(B[0], B[1], B[2], B[3]);
    }
    if (len(B) == IPv6len) {
        return *this;
    }
    return {};
}

// Default route masks for IPv4.
static const IPMask classAMask = IPv4Mask(0xff, 0, 0, 0);
static const IPMask classBMask = IPv4Mask(0xff, 0xff, 0, 0);
static const IPMask classCMask = IPv4Mask(0xff, 0xff, 0xff, 0);

// DefaultMask returns the default IP mask for the IP address ip.
// Only IPv4 addresses have default masks; DefaultMask returns
// nil if ip is not a valid IPv4 address.
IPMask IP::DefaultMask() const {
    IP ip4 = To4();
    if (!ip4) {
        return {};
    }
    if (B[0] < 0x80) {
        return classAMask;
    } else if (B[0] < 0xC0) {
        return classBMask;
    } else {
        return classCMask;
    }
}

namespace xx {
static bool allFF(const bytez<>& b) {
    for (int i = 0; i < len(b); i++) {
        if (b[i] != 0xff) {
            return false;
        }
    }
    return true;
}
}  // namespace xx

// Mask returns the result of masking the IP address ip with mask.
IP IP::Mask(IPMask mask) {
    if (len(mask.B) == IPv6len && len(B) == IPv4len && xx::allFF(mask.B(0, 12))) {
        mask.B = mask.B(12);
    }
    if (len(mask.B) == IPv4len && len(B) == IPv6len && bytes::Equal(B(0, 12), v4InV6Prefix)) {
        B = B(12);
    }
    int n = len(B);
    if (n != len(mask.B)) {
        return {};
    }
    bytez<> out = make(n);
    for (int i = 0; i < n; i++) {
        out[i] = B[i] & mask.B[i];
    }
    return IP(out);
}

namespace xx {
// ubtoa encodes the string form of the integer v to dst[start:] and
// returns the number of bytes written to dst. The caller must ensure
// that dst has sufficient length.
static int ubtoa(bytez<> dst, int start, byte v) {
    if (v < 10) {
        dst[start] = v + '0';
        return 1;
    } else if (v < 100) {
        dst[start + 1] = v % 10 + '0';
        dst[start] = v / 10 + '0';
        return 2;
    }

    dst[start + 2] = v % 10 + '0';
    dst[start + 1] = (v / 10) % 10 + '0';
    dst[start] = v / 100 + '0';
    return 3;
}

static string hexString(const bytez<>& b) {
    bytez<> s = make(len(b) * 2);
    for (int i = 0; i < len(b); i++) {
        auto tn = b[i];
        s[i * 2] = hexDigit[tn >> 4];
        s[i * 2 + 1] = hexDigit[tn & 0xf];
    }
    return string(s);
}

// ipEmptyString is like ip.String except that it returns
// an empty string when ip is unset.
static string ipEmptyString(const IP& ip) {
    if (len(ip.B) == 0) {
        return "";
    }
    return ip.String();
}

// Convert i to a hexadecimal string. Leading zeros are not printed.
static bytez<> appendHex(bytez<> dst, uint32 i) {
    if (i == 0) {
        return append(dst, '0');
    }
    for (int j = 7; j >= 0; j--) {
        int v = i >> uint(j * 4);
        if (v > 0) {
            dst = append(dst, hexDigit[v & 0xf]);
        }
    }
    return dst;
}

// If mask is a sequence of 1 bits followed by 0 bits,
// return the number of 1 bits.
static int simpleMaskLength(const IPMask& mask) {
    int n = 0;
    for (int i = 0; i < len(mask.B); i++) {
        auto v = mask.B[i];
        if (v == 0xff) {
            n += 8;
            continue;
        }
        // found non-ff byte
        // count 1 bits
        while ((v & 0x80) != 0) {
            n++;
            v <<= 1;
        }
        // rest must be 0 bits
        if (v != 0) {
            return -1;
        }
        for (i++; i < len(mask.B); i++) {
            if (mask.B[i] != 0) {
                return -1;
            }
        }
        break;
    }
    return n;
}

}  // namespace xx

// String ...
string IP::String() const {
    if (len(B) == 0) {
        return "<nil>";
    }

    IP p4 = To4();
    if (len(p4.B) == IPv4len) {
        return fmt::Sprintf("%d.%d.%d.%d", p4.B[0], p4.B[1], p4.B[2], p4.B[3]);
    } else if (len(B) != IPv6len) {
        return "?" + xx::hexString(B);
    }

    auto& p = B;
    // Find longest run of zeros.
    int e0 = -1, e1 = -1;
    for (int i = 0; i < IPv6len; i += 2) {
        int j = i;
        while (j < IPv6len && p[j] == 0 && p[j + 1] == 0) {
            j += 2;
        }
        if (j > i && (j - i) > (e1 - e0)) {
            e0 = i;
            e1 = j;
            i = j;
        }
    }
    // The symbol "::" MUST NOT be used to shorten just one 16 bit 0 field.
    if ((e1 - e0) <= 2) {
        e0 = -1;
        e1 = -1;
    }

    const int maxLen = strlen("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    auto b = make(0, maxLen);

    // Print with possible :: in place of run of zeros
    for (int i = 0; i < IPv6len; i += 2) {
        if (i == e0) {
            b = append(b, ':', ':');
            i = e1;
            if (i >= IPv6len) {
                break;
            }
        } else if (i > 0) {
            b = append(b, ':');
        }
        b = xx::appendHex(b, (uint32(p[i]) << 8) | uint32(p[i + 1]));
    }
    return string(b);
}

// Size returns the number of leading ones and total bits in the mask.
// If the mask is not in the canonical form--ones followed by zeros--then
// Size returns 0, 0.
R<int /*ones*/, int /*bits*/> IPMask::Size() const {
    int ones = xx::simpleMaskLength(*this), bits = len(B) * 8;
    if (ones == -1) {
        return {0, 0};
    }
    return {ones, bits};
}

// String returns the hexadecimal form of m, with no punctuation.
string IPMask::String() const {
    if (len(B) == 0) {
        return "<nil>";
    }
    return xx::hexString(B);
}

// ParseError ...
static error ParseError(const string& typ, const string& txt) {
    return fmt::Errorf("invali %s:%s", typ.c_str(), txt.c_str());
}

// ParseCIDR parses s as a CIDR notation IP address and prefix length,
// like "192.0.2.0/24" or "2001:db8::/32", as defined in
// RFC 4632 and RFC 4291.
//
// It returns the IP address and the network implied by the IP and
// prefix length.
// For example, ParseCIDR("192.0.2.1/24") returns the IP address
// 192.0.2.1 and the network 192.0.2.0/24.
R<IP, Ref<IPNet>, error> ParseCIDR(const string& s) {
    int i = strings::IndexByte(s, '/');
    if (i < 0) {
        return {{}, nil, ParseError("CIDR address", s)};
    }
    auto addr = s.substr(0, i);
    auto mask = s.substr(i + 1);
    int iplen = IPv4len;
    auto ip = xx::parseIPv4(addr);
    if (!ip) {
        iplen = IPv6len;
        ip = xx::parseIPv6(addr);
    }
    AUTO_R(n, j, ok, xx::dtoi(mask));
    if (!ip || !ok || j != len(mask) || n < 0 || n > 8 * iplen) {
        return {{}, nil, ParseError("CIDR address", s)};
    }
    auto ipnet = NewRef<IPNet>();
    ipnet->IP = ip;
    ipnet->IPMask = CIDRMask(n, 8 * iplen);
    return {ip, ipnet, nil};
}

}  // namespace net
}  // namespace gx
