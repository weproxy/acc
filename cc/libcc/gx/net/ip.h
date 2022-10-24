//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"

namespace gx {
namespace net {

constexpr int IPv4len = 4;
constexpr int IPv6len = 16;

////////////////////////////////////////////////////////////////////////////////
//

// IP ...
struct IP final {
    bytez<> B;

    IP(const void* p = nil){};
    IP(const bytez<> b) : B(b) {}
    IP(const IP& ip) : B(ip.B) {}

    uint8 len() const { return B.size(); }

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

    IP To4() const;
    IP To16() const;
};

// IPMask ...
struct IPMask {
    bytez<> B;

    string String();
};

// IPNet ...
struct IPNet {
    IP ip;
    IPMask mask;

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

// IPv4Mask returns the IP mask (in 4-byte form) of the
// IPv4 mask a.b.c.d.
IPMask IPv4Mask(uint8 a, uint8 b, uint8 c, uint8 d);

// ParseIP ...
IP ParseIP(const string& s);

}  // namespace net
}  // namespace gx
