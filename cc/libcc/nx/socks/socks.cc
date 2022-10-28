//
// weproxy@foxmail.com 2022/10/03
//

#include "socks.h"

#include "gx/errors/errors.h"
#include "logx/logx.h"

namespace nx {
namespace socks {

////////////////////////////////////////////////////////////////////////////////
// error ...
const error ErrUnrecognizedAddrType = errors::New("unrecognized address type");
const error ErrInvalidSocksVersion = errors::New("invalid socks version");
const error ErrUserAuthFailed = errors::New("user authentication failed");
const error ErrNoSupportedAuth = errors::New("no supported authentication mechanism");

const error ErrInvalidAddrType = errors::New("invalid address type");
const error ErrInvalidAddrLen = errors::New("invalid address length");

////////////////////////////////////////////////////////////////////////////////
// IP ...
net::IP Addr::IP() const {
    if (len(B) > 0) {
        switch (AddrType(B[0])) {
            case AddrType::IPv4:
                return net::IP(B(1, 1 + net::IPv4len));
            case AddrType::IPv6:
                return net::IP(B(1, 1 + net::IPv6len));
            default:
                break;
        }
    }
    return net::IP{};
}

// Port ...
int Addr::Port() const {
    if (len(B) > 0) {
        switch (AddrType(B[0])) {
            case AddrType::IPv4:
                return int(B[1 + net::IPv4len]) << 8 | int(B[1 + net::IPv4len + 1]);
            case AddrType::IPv6:
                return int(B[1 + net::IPv6len]) << 8 | int(B[1 + net::IPv6len + 1]);
            case AddrType::Domain:
                return int(B[2 + B[1]]) << 8 | int(B[2 + B[1] + 1]);
            default:
                break;
        }
    }
    return 0;
}

// ToNetAddr ...
net::Addr Addr::ToNetAddr() const { return net::MakeAddr(IP(), Port()); }

// FromNetAddr ...
void Addr::FromNetAddr(net::Addr addr) {
    auto ip4 = addr->IP.To4();
    if (ip4) {
        B = make(1 + net::IPv4len + 2);
        B[0] = byte(AddrType::IPv4);
        copy(B(1, 1 + net::IPv4len), ip4.B);
        B[1 + net::IPv4len] = uint8(addr->Port >> 8);
        B[1 + net::IPv4len + 1] = uint8(addr->Port);
        return;
    }

    auto ip6 = addr->IP.To16();
    B = make(1 + net::IPv6len + 2);
    B[0] = byte(AddrType::IPv6);
    copy(B(1, 1 + net::IPv6len), ip6.B);
    B[1 + net::IPv6len] = uint8(addr->Port >> 8);
    B[1 + net::IPv6len + 1] = uint8(addr->Port);
}

// String ...
const string Addr::String() const {
    if (len(B) <= 0) {
        return "<nil>";
    }

    return GX_SS(IP() << ":" << Port());
}

////////////////////////////////////////////////////////////////////////////////
// ParseAddr ...
//	| ATYP | ADDR | PORT |
//	+------+------+------+
//	|  1   |  x   |  2   |
R<Ref<Addr>, error> ParseAddr(const bytez<> B) {
    if (len(B) < 1 + 1 + 1 + 2) {
        return {nil, ErrInvalidAddrLen};
    }

    int m = 0;
    if (AddrType::IPv4 == AddrType(B[0])) {
        m = 1 + net::IPv4len + 2;
    } else if (AddrType::IPv6 == AddrType(B[0])) {
        m = 1 + net::IPv6len + 2;
    } else if (AddrType::Domain == AddrType(B[0])) {
        m = 1 + 1 + B[1] + 2;  // DOMAIN_LEN = B[1]
    } else {
        return {nil, ErrInvalidAddrLen};
    }

    if (len(B) < m) {
        return {nil, ErrInvalidAddrLen};
    }
    return {NewRef<Addr>(B(0, m)), nil};
}

////////////////////////////////////////////////////////////////////////////////
// serverAuth ...
template <typename ReadWriter, typename std::enable_if<io::xx::has_read_write<ReadWriter>::value, int>::type = 0>
static error clientAuth(ReadWriter c, bool userPassRequired, const ProvideUserPassFn& provideUserPassFn = {}) {
    return nil;
}

// ClientHandshake ...
R<Ref<Addr>, error> ClientHandshake(net::Conn c, Command cmd, net::Addr addr,
                                    const ProvideUserPassFn& provideUserPassFn) {
    return {nil, nil};
}

////////////////////////////////////////////////////////////////////////////////
// serverAuth ...
template <typename ReadWriter, typename std::enable_if<io::xx::has_read_write<ReadWriter>::value, int>::type = 0>
static error serverAuth(ReadWriter c, bool userPassRequired, const CheckUserPassFn& checkUserPassFn = {}) {
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

// ServerHandshake ...
R<Command, Ref<Addr>, error> ServerHandshake(net::Conn c, const CheckUserPassFn& checkUserPassFn) {
    c->SetDeadline(time::Now().Add(time::Second * 5));
    DEFER(c->SetDeadline(time::Time{}));

    Command cmd = Command(0);
    bytez<> buf = make(256);

    // >>> REQ:
    //     | VER | NMETHODS | METHODS  |
    //     +-----+----------+----------+
    //     |  1  |    1     | 1 to 255 |
    AUTO_R(_1, er1, io::ReadFull(c, buf(0, 2)));
    if (er1) {
        WriteReply(c, Reply::AuthFailure);
        return {cmd, nil, er1};
    }

    auto methodCnt = buf[1];

    if (buf[0] != Version5 || methodCnt == 0 || methodCnt > 16) {
        WriteReply(c, Reply::AuthFailure);
        return {cmd, nil, ErrInvalidSocksVersion};
    }

    AUTO_R(n, er2, io::ReadFull(c, buf(0, methodCnt)));
    if (er2) {
        WriteReply(c, Reply::AuthFailure);
        return {cmd, nil, er2};
    }

    bool methodNotRequired = false, methodUserPass = false;
    for (int i = 0; i < n; i++) {
        switch (buf[i]) {
            case AuthMethodNotRequired:
                methodNotRequired = true;
                break;
            case AuthMethodUserPass:
                methodUserPass = true;
                break;
            default:
                break;
        }
    }

    if (methodNotRequired || methodUserPass) {
        // <<< REP:
        //     | VER | METHOD |
        //     +-----+--------+
        //     |  1  |   1    |

        // >>> REQ:
        //     | VER | ULEN |  UNAME   | PLEN |  PASSWD  |
        //     +-----+------+----------+------+----------+
        //     |  1  |  1   | 1 to 255 |  1   | 1 to 255 |

        // <<< REP:
        //     | VER | STATUS |
        //     +-----+--------+
        //     |  1  |   1    |
        auto err = serverAuth(c, methodUserPass, checkUserPassFn);
        if (err) {
            return {cmd, nil, err};
        }
    } else {
        WriteReply(c, Reply::NoAcceptableMethods);
        return {cmd, nil, ErrNoSupportedAuth};
    }

    // >>> REQ:
    //     | VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |
    AUTO_R(_3, er3, io::ReadFull(c, buf(0, 3)));
    if (er3) {
        WriteReply(c, Reply::AddressNotSupported);
        return {cmd, nil, er3};
    }

    auto ver = buf[0];
    cmd = Command(buf[1]);
    auto rsv = buf[2];

    if (Command::Connect != cmd && Command::Assoc != cmd) {
        WriteReply(c, Reply::CommandNotSupported);
        return {cmd, nil, ToError(Reply::CommandNotSupported)};
    }

    AUTO_R(_4, raddr, er4, ReadAddr(c));
    if (er4) {
        WriteReply(c, Reply::AddressNotSupported);
        return {cmd, nil, er4};
    }

    return {cmd, raddr, nil};
}

}  // namespace socks
}  // namespace nx
