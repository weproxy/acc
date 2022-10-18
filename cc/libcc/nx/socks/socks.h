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
enum Command {
    CmdConnect = 0x01,
    CmdBind = 0x02,
    CmdAssociate = 0x03,
};

// ToString ...
inline const char* ToString(Command cmd) {
    switch (cmd) {
        case CmdConnect:
            return "CmdConnect";
        case CmdBind:
            return "CmdBind";
        case CmdAssociate:
            return "CmdAssociate";
        default:
            return "<CmdUnknow>";
    }
}

////////////////////////////////////////////////////////////////////////////////
// Method ...
enum Method {
    AuthMethodNotRequired = 0x00,          // no authentication required
    AuthMethodUserPass = 0x02,             // use username/password
    AuthMethodNoAcceptableMethods = 0xff,  // no acceptable authentication methods

};

////////////////////////////////////////////////////////////////////////////////
// Reply
enum Reply {
    ReplyAuthSuccess = 0,
    ReplyAuthFailure = 1,

    ReplySuccess = 0,
    ReplyGeneralFailure = 1,
    ReplyConnectionNotAllowed = 2,
    ReplyNetworkUnreachable = 3,
    ReplyHostUnreachable = 4,
    ReplyConnectionRefused = 5,
    ReplyTTLExpired = 6,
    ReplyCommandNotSupported = 7,
    ReplyAddressNotSupported = 8,
};

// ToString ...
inline const char* ToString(Reply e) {
    switch (e) {
        case ReplySuccess:
            return "(0)Succeeded";
        case ReplyGeneralFailure:
            return "(1)General socks server failure";
        case ReplyConnectionNotAllowed:
            return "(2)Connection not allowed by ruleset";
        case ReplyNetworkUnreachable:
            return "(3)Network unreachable";
        case ReplyHostUnreachable:
            return "(4)Host unreachable";
        case ReplyConnectionRefused:
            return "(5)Connection refused";
        case ReplyTTLExpired:
            return "(6)TTL expired";
        case ReplyCommandNotSupported:
            return "(7)Command not supported";
        case ReplyAddressNotSupported:
            return "(8)Address type not supported";
        default:
            return "<socks error>";
    }
}

// ToError ...
inline error ToError(Reply e) { return errors::New(ToString(e)); }

////////////////////////////////////////////////////////////////////////////////
// error ...
extern const error ErrUnrecognizedAddrType;
extern const error ErrInvalidSocksVersion;
extern const error ErrUserAuthFailed;
extern const error ErrNoSupportedAuth;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// CheckUserPassFn ...
using CheckUserPassFn = func<error(const string& user, const string& pass)>;

// ServerAuth ...
template <typename ReadWriter, typename std::enable_if<io::xx::has_read_write<ReadWriter>::value, int>::type = 0>
error ServerAuth(ReadWriter c, bool userPassRequired, const CheckUserPassFn& checkUserPassFn = {}) {
    slice<byte> buf = make(512);

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
    buf[1] = err ? ReplyAuthFailure : ReplyAuthSuccess;
    AUTO_R(_5, er5, c->Write(buf(0, 2)));
    if (er5) {
        return er5;
    }

    return err;
}

// WriteReply ...
template <typename Writer, typename std::enable_if<io::xx::has_write<Writer>::value, int>::type = 0>
error WriteReply(Writer w, uint8 reply, uint8 resverd = 0, net::Addr boundAddr = nil) {
    // <<< REP:
    //     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |

    slice<byte> buf = make(boundAddr ? 3 + MaxAddrLen : 8);
    int len = 3;

    buf[0] = Version5;
    buf[1] = reply;
    buf[2] = resverd;

    if (boundAddr) {
        uint8* B = buf.data() + len;

        auto ip4 = boundAddr->IP.To4();
        if (ip4) {
            B[0] = AddrTypeIPv4;
            memcpy(B + 1, ip4.B, net::IPv4len);
            B[1 + net::IPv4len] = uint8(boundAddr->Port >> 8);
            B[1 + net::IPv4len + 1] = uint8(boundAddr->Port);
            len += 1 + net::IPv4len + 2;
        } else {
            auto ip6 = boundAddr->IP.To16();
            B[0] = AddrTypeIPv6;
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
