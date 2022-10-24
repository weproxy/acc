//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "addr.h"
#include "gx/gx.h"

namespace nx {
namespace socks {
using namespace gx;

////////////////////////////////////////////////////////////////////////////////
// Version5  ...
const uint8 Version5 = 0x05;

// UserAuthVersion ...
const uint8 UserAuthVersion = 0x01;

////////////////////////////////////////////////////////////////////////////////
// Command ...
enum class Command : uint8 {
    Connect = 0x01,
    Bind = 0x02,
    Associate = 0x03,
};

// ToString ...
inline const char* ToString(const Command e) {
#define CASE_RETURN_COMMAND(e) \
    case Command::e:           \
        return "Cmd" #e

    switch (e) {
        CASE_RETURN_COMMAND(Connect);
        CASE_RETURN_COMMAND(Bind);
        CASE_RETURN_COMMAND(Associate);
        default:
            return "";
    }
}

////////////////////////////////////////////////////////////////////////////////
// Method ...
enum Method {
    AuthMethodNotRequired = 0x00,  // no authentication required
    AuthMethodUserPass = 0x02,     // use username/password
};

////////////////////////////////////////////////////////////////////////////////
// Reply
enum class Reply : uint8 {
    AuthSuccess = 0,
    AuthFailure = 1,

    Success = 0,
    GeneralFailure = 1,
    ConnectionNotAllowed = 2,
    NetworkUnreachable = 3,
    HostUnreachable = 4,
    ConnectionRefused = 5,
    TTLExpired = 6,
    CommandNotSupported = 7,
    AddressNotSupported = 8,

    NoAcceptableMethods = 0xff,  // no acceptable authentication methods
};

// ToString ...
inline const char* ToString(const Reply e) {
    switch (e) {
        case Reply::Success:
            return "(0)Succeeded";
        case Reply::GeneralFailure:
            return "(1)General socks server failure";
        case Reply::ConnectionNotAllowed:
            return "(2)Connection not allowed by ruleset";
        case Reply::NetworkUnreachable:
            return "(3)Network unreachable";
        case Reply::HostUnreachable:
            return "(4)Host unreachable";
        case Reply::ConnectionRefused:
            return "(5)Connection refused";
        case Reply::TTLExpired:
            return "(6)TTL expired";
        case Reply::CommandNotSupported:
            return "(7)Command not supported";
        case Reply::AddressNotSupported:
            return "(8)Address type not supported";
        default:
            return "<socks error>";
    }
}

// ToError ...
inline error ToError(const Reply e) { return errors::New(ToString(e)); }

////////////////////////////////////////////////////////////////////////////////
// error ...
extern const error ErrUnrecognizedAddrType;
extern const error ErrInvalidSocksVersion;
extern const error ErrUserAuthFailed;
extern const error ErrNoSupportedAuth;

////////////////////////////////////////////////////////////////////////////////

// CheckUserPassFn ...
using CheckUserPassFn = func<error(const string& user, const string& pass)>;

// ServerAuth ...
template <typename ReadWriter, typename std::enable_if<io::xx::has_read_write<ReadWriter>::value, int>::type = 0>
error ServerAuth(ReadWriter c, bool userPassRequired, const CheckUserPassFn& checkUserPassFn = {}) {
    bytez<> buf = make(512);

    // <<< REP:
    //     | VER | METHOD |
    //     +-----+--------+
    //     |  1  |   1    |

    buf[0] = Version5;
    buf[1] = userPassRequired ? AuthMethodUserPass : AuthMethodNotRequired;
    AUTO_R(_1, er1, c->Write(buf(0, 2)));
    if (er1) {
        return er1;
    }

    if (!userPassRequired) {
        return nil;
    }

    // >>> REQ:
    //     | VER | ULEN |  UNAME   | PLEN |  PASSWD  |
    //     +-----+------+----------+------+----------+
    //     |  1  |  1   | 1 to 255 |  1   | 1 to 255 |

    AUTO_R(_2, er2, io::ReadFull(c, buf(0, 2)));
    if (er2) {
        return er2;
    }
    if (buf[0] != UserAuthVersion) {
        return fmt::Errorf("invalid auth version: %d", buf[0]);
    }

    auto userLen = buf[1];
    if (userLen > len(buf) - 2) {
        return ErrUserAuthFailed;
    }
    AUTO_R(_3, er3, io::ReadFull(c, buf(0, userLen + 1)));
    if (er3) {
        return er3;
    }
    string user((char*)buf.data(), userLen);

    auto passLen = buf[userLen];
    if (passLen > len(buf) - 2) {
        return ErrUserAuthFailed;
    }
    AUTO_R(_4, er4, io::ReadFull(c, buf(0, passLen)));
    if (er4) {
        return er4;
    }
    string pass((char*)buf.data(), passLen);

    // check user pass
    auto err = checkUserPassFn ? checkUserPassFn(user, pass) : nil;

    // <<< REP:
    //     | VER | STATUS |
    //     +-----+--------+
    //     |  1  |   1    |

    buf[0] = UserAuthVersion;
    buf[1] = err ? byte(Reply::AuthFailure) : byte(Reply::AuthSuccess);
    AUTO_R(_5, er5, c->Write(buf(0, 2)));
    if (er5) {
        return er5;
    }

    return err;
}

// WriteReply ...
template <typename Writer, typename std::enable_if<io::xx::has_write<Writer>::value, int>::type = 0>
error WriteReply(Writer w, Reply reply, uint8 resverd = 0, net::Addr boundAddr = nil) {
    // <<< REP:
    //     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |

    bytez<> buf = make(boundAddr ? 3 + MaxAddrLen : 8);
    int len = 3;

    buf[0] = Version5;
    buf[1] = byte(reply);
    buf[2] = resverd;

    if (boundAddr) {
        uint8* B = buf.data() + len;

        auto ip4 = boundAddr->IP.To4();
        if (ip4) {
            B[0] = byte(AddrType::IPv4);
            memcpy(B + 1, ip4.B, net::IPv4len);
            B[1 + net::IPv4len] = uint8(boundAddr->Port >> 8);
            B[1 + net::IPv4len + 1] = uint8(boundAddr->Port);
            len += 1 + net::IPv4len + 2;
        } else {
            auto ip6 = boundAddr->IP.To16();
            B[0] = byte(AddrType::IPv6);
            memcpy(B + 1, ip6.B, net::IPv6len);
            B[1 + net::IPv6len] = uint8(boundAddr->Port >> 8);
            B[1 + net::IPv6len + 1] = uint8(boundAddr->Port);
            len += 1 + net::IPv6len + 2;
        }
    }

    AUTO_R(_, err, w->Write(buf(0, len)));

    return err;
}

}  // namespace socks
}  // namespace nx


////////////////////////////////////////////////////////////////////////////////
namespace std {
// override ostream <<
inline ostream& operator<<(ostream& o, const nx::socks::Command v) { return o << nx::socks::ToString(v); }
inline ostream& operator<<(ostream& o, const nx::socks::Reply v) { return o << nx::socks::ToString(v); }
}  // namespace std
