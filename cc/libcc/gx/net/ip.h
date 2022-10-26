//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"

namespace gx {
namespace net {

constexpr int IPv4len = 4;
constexpr int IPv6len = 16;

constexpr const char* hexDigit = "0123456789abcdef";

////////////////////////////////////////////////////////////////////////////////
//

// IPMask ...
struct IPMask {
    bytez<> B;

    // Size returns the number of leading ones and total bits in the mask.
    // If the mask is not in the canonical form--ones followed by zeros--then
    // Size returns 0, 0.
    R<int /*ones*/, int /*bits*/> Size() const;

    // String returns the hexadecimal form of m, with no punctuation.
    string String() const;
};

// IP ...
struct IP final {
    bytez<> B;

    IP(const void* p = nil){};
    IP(const bytez<> b) : B(b) {}
    IP(const IP& ip) : B(ip.B) {}

    operator bool() const { return bool(B); }

    byte& operator[](int i) { return B[i]; }
    byte operator[](int i) const { return B[i]; }

    // x == nil or x != nil
    bool operator==(const void* p) const { return p == nil && !bool(B); }
    bool operator!=(const void* p) const { return p == nil && bool(B); }

    string String() const;

    // Equal reports whether ip and x are the same IP address.
    // An IPv4 address and that same address in IPv6 form are
    // considered to be equal.
    bool Equal(const IP& x) const;

    // IsUnspecified reports whether ip is an unspecified address, either
    // the IPv4 address "0.0.0.0" or the IPv6 address "::".
    bool IsUnspecified() const;

    // IsLoopback reports whether ip is a loopback address.
    bool IsLoopback() const;

    // IsPrivate reports whether ip is a private address, according to
    // RFC 1918 (IPv4 addresses) and RFC 4193 (IPv6 addresses).
    bool IsPrivate() const;

    // IsMulticast reports whether ip is a multicast address.
    bool IsMulticast() const;

    // IsInterfaceLocalMulticast reports whether ip is
    // an interface-local multicast address.
    bool IsInterfaceLocalMulticast() const;

    // IsLinkLocalMulticast reports whether ip is a link-local
    // multicast address.
    bool IsLinkLocalMulticast() const;

    // IsLinkLocalUnicast reports whether ip is a link-local
    // unicast address.
    bool IsLinkLocalUnicast() const;

    // IsGlobalUnicast reports whether ip is a global unicast
    // address.
    //
    // The identification of global unicast addresses uses address type
    // identification as defined in RFC 1122, RFC 4632 and RFC 4291 with
    // the exception of IPv4 directed broadcast addresses.
    // It returns true even if ip is in IPv4 private address space or
    // local IPv6 unicast address space.
    bool IsGlobalUnicast() const;

    // To4 converts the IPv4 address ip to a 4-byte representation.
    // If ip is not an IPv4 address, To4 returns nil.
    IP To4() const;

    // To16 converts the IP address ip to a 16-byte representation.
    // If ip is not an IP address (it is the wrong length), To16 returns nil.
    IP To16() const;

    // DefaultMask returns the default IP mask for the IP address ip.
    // Only IPv4 addresses have default masks; DefaultMask returns
    // nil if ip is not a valid IPv4 address.
    IPMask DefaultMask() const;

    // Mask returns the result of masking the IP address ip with mask.
    IP Mask(IPMask mask);
};

// IPNet ...
struct IPNet {
    IP IP;
    IPMask IPMask;

    string String();
};

////////////////////////////////////////////////////////////////////////////////
//
// Well-known IPv4 addresses
extern const IP IPv4bcast;      // limited broadcast
extern const IP IPv4allsys;     // all systems
extern const IP IPv4allrouter;  // all routers
extern const IP IPv4zero;       // all zeros

// Well-known IPv6 addresses
extern const IP IPv6zero;
extern const IP IPv6unspecified;
extern const IP IPv6loopback;
extern const IP IPv6interfacelocalallnodes;
extern const IP IPv6linklocalallnodes;
extern const IP IPv6linklocalallrouters;

////////////////////////////////////////////////////////////////////////////////
//

// IPv4 returns the IP address (in 16-byte form) of the
// IPv4 address a.b.c.d.
IP IPv4(uint8 a, uint8 b, uint8 c, uint8 d);

// makeIPv6 ...
namespace xx {
IP makeIPv6(uint8 a0, uint8 a1, uint8 a14, uint8 a15);
}  // namespace xx

// IPv4Mask returns the IP mask (in 4-byte form) of the
// IPv4 mask a.b.c.d.
IPMask IPv4Mask(uint8 a, uint8 b, uint8 c, uint8 d);

// CIDRMask returns an IPMask consisting of 'ones' 1 bits
// followed by 0s up to a total length of 'bits' bits.
// For a mask of this form, CIDRMask is the inverse of IPMask.Size.
IPMask CIDRMask(int ones, int bits);

// ParseIP parses s as an IP address, returning the result.
// The string s can be in IPv4 dotted decimal ("192.0.2.1"), IPv6
// ("2001:db8::68"), or IPv4-mapped IPv6 ("::ffff:192.0.2.1") form.
// If s is not a valid textual representation of an IP address,
// ParseIP returns nil.
IP ParseIP(const string& s);

// ParseCIDR parses s as a CIDR notation IP address and prefix length,
// like "192.0.2.0/24" or "2001:db8::/32", as defined in
// RFC 4632 and RFC 4291.
//
// It returns the IP address and the network implied by the IP and
// prefix length.
// For example, ParseCIDR("192.0.2.1/24") returns the IP address
// 192.0.2.1 and the network 192.0.2.0/24.
R<IP, Ref<IPNet>, error> ParseCIDR(const string& s);

}  // namespace net
}  // namespace gx
