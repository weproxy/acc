//
// weproxy@foxmail.com 2022/10/03
//

#include "s5.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/socks/socks.h"

namespace app {
namespace proto {
namespace s5 {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////

// checkUserPass ...
static error checkUserPass(const string& user, const string& pass) {
    LOGS_D(TAG << " handshake() auth: user=" << user << ", pass=" << pass);
    return nil;
}

// handshake ...
static R<socks::Command, Ref<socks::Addr>, error> handshake(net::Conn c) {
    c->SetDeadline(time::Now().Add(time::Second * 5));
    DEFER(c->SetDeadline(time::Time{}));

    socks::Command cmd = socks::Command(0);
    bytez<> buf = make(256);

    // >>> REQ:
    //     | VER | NMETHODS | METHODS  |
    //     +-----+----------+----------+
    //     |  1  |    1     | 1 to 255 |

    AUTO_R(_1, er1, io::ReadFull(c, buf(0, 2)));
    if (er1) {
        socks::WriteReply(c, socks::ReplyAuthFailure);
        return {cmd, nil, er1};
    }

    auto methodCnt = buf[1];

    if (buf[0] != socks::Version5 || methodCnt == 0 || methodCnt > 16) {
        socks::WriteReply(c, socks::ReplyAuthFailure);
        return {cmd, nil, socks::ErrInvalidSocksVersion};
    }

    AUTO_R(n, er2, io::ReadFull(c, buf(0, methodCnt)));
    if (er2) {
        socks::WriteReply(c, socks::ReplyAuthFailure);
        return {cmd, nil, er2};
    }

    bool methodNotRequired = false, methodUserPass = false;
    for (int i = 0; i < n; i++) {
        switch (buf[i]) {
            case socks::AuthMethodNotRequired:
                methodNotRequired = true;
                break;
            case socks::AuthMethodUserPass:
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
        auto err = socks::ServerAuth(c, methodUserPass, checkUserPass);
        if (err) {
            return {cmd, nil, err};
        }
    } else {
        socks::WriteReply(c, socks::AuthMethodNoAcceptableMethods);
        return {cmd, nil, socks::ErrNoSupportedAuth};
    }

    // >>> REQ:
    //     | VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |

    AUTO_R(_3, er3, io::ReadFull(c, buf(0, 3)));
    if (er3) {
        socks::WriteReply(c, socks::ReplyAddressNotSupported);
        return {cmd, nil, er3};
    }

    auto ver = buf[0];
    cmd = socks::Command(buf[1]);
    auto rsv = buf[2];

    if (socks::CmdConnect != cmd && socks::CmdAssociate != cmd) {
        socks::WriteReply(c, socks::ReplyCommandNotSupported);
        return {cmd, nil, socks::ToError(socks::ReplyCommandNotSupported)};
    }

    AUTO_R(_4, raddr, er4, socks::ReadAddr(c));
    if (er4) {
        socks::WriteReply(c, socks::ReplyAddressNotSupported);
        return {cmd, nil, er4};
    }

    return {cmd, raddr, nil};
}

////////////////////////////////////////////////////////////////////////////////
// handleTCP ...
extern error handleTCP(net::Conn c, net::Addr raddr);

// handleAssoc ...
extern error handleAssoc(net::Conn c, net::Addr raddr);

////////////////////////////////////////////////////////////////////////////////
// handleConn ...
static error handleConn(net::Conn c) {
    DEFER(c->Close());

    AUTO_R(cmd, raddr, err, handshake(c));
    if (err || !raddr) {
        auto er = err ? err : socks::ErrInvalidAddrType;
        LOGS_E(TAG << " handshake(), err: " << er);
        return er;
    }

    // LOGS_D(TAG << " handshake(), cmd=" << socks::ToString(cmd) << ", raddr=" << raddr);

    switch (cmd) {
        case socks::CmdConnect:
            return handleTCP(c, raddr->ToNetAddr());
        case socks::CmdAssociate:
            return handleAssoc(c, raddr->ToNetAddr());
        case socks::CmdBind:
            return errors::New("not support socks command: bind");
        default:
            return fmt::Errorf("unknow socks command: %d", cmd);
    }
}

////////////////////////////////////////////////////////////////////////////////
// server_t ...
struct server_t : public proto::server_t {
    string addr_;
    net::Listener ln_;

    server_t(const string& addr) : addr_(addr) {}

    ////////////////////////////////////////////////////////////////////////////////
    //  Start ...
    virtual error Start() override {
        AUTO_R(ln, err, net::Listen(addr_));
        if (err) {
            LOGS_E(TAG << " Listen(" << addr_ << "), err: " << err);
            return err;
        }

        LOGS_D(TAG << " Start(" << addr_ << ")");

        ln_ = ln;
        gx::go([ln] {
            for (;;) {
                AUTO_R(c, err, ln->Accept());
                if (err) {
                    if (err != net::ErrClosed) {
                        LOGS_E(TAG << " Accept(), err: " << err);
                    }
                    break;
                }

                LOGS_V(TAG << " Accept() " << c->RemoteAddr());

                gx::go([c] { handleConn(c); });
            }
        });

        return nil;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Close ...
    virtual error Close() override {
        if (ln_) {
            ln_->Close();
        }
        LOGS_D(TAG << " Close()");
        return nil;
    }
};

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j) {
    LOGS_V(TAG << " Conf: " << j);

    auto addr = j["listen"];
    if (!addr.is_string() || addr.empty()) {
        return {nil, errors::New("invalid addr")};
    }

    return {NewRef<server_t>(string(addr)), nil};
}

////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    // LOGS_V(TAG << " init()");
    proto::Register("s5", New);
    return true;
}();

}  // namespace s5
}  // namespace proto
}  // namespace app
