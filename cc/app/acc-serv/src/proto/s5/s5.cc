//
// weproxy@foxmail.com 2022/10/03
//

#include "s5.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/socks/socks.h"

NAMESPACE_BEG_S5
using namespace nx;

namespace xx {
////////////////////////////////////////////////////////////////////////////////////////////////////

// checkUserPass ...
static error checkUserPass(const string& user, const string& pass) {
    // LOGS_D(TAG << " handshake() auth: user=" << user << ", pass=" << pass);
    return nil;
}

// handshake ...
static R<socks::Command, socks::Addr, error> handshake(net::Conn c) {
    c->SetDeadline(time::Now().Add(time::Second * 3));
    DEFER(c->SetDeadline(time::Time{}));

    socks::Command cmd = socks::Command(0);
    uint8 buf[256];

    AUTO_R(_1, er1, io::ReadFull(c, buf, 2));
    if (er1) {
        socks::WriteReply(c, socks::ReplyAuthFailure);
        return {cmd, nil, er1};
    }

    auto methodCnt = buf[1];

    if (buf[0] != socks::Version5 || methodCnt == 0 || methodCnt > 16) {
        socks::WriteReply(c, socks::ReplyAuthFailure);
        return {cmd, nil, socks::ErrInvalidSocksVersion};
    }

    AUTO_R(n, er2, io::ReadFull(c, buf, methodCnt));
    if (er2) {
        socks::WriteReply(c, socks::ReplyAuthFailure);
        return {cmd, nil, er2};
    }

    bool methodUserPass = false;

    for (int i = 0; i < n; i++) {
        switch (buf[i]) {
            case socks::AuthMethodNotRequired:
                break;
            case socks::AuthMethodUserPass:
                methodUserPass = true;
                break;
            default:
                break;
        }
    }

    if (!methodUserPass) {
        socks::WriteReply(c, socks::AuthMethodNoAcceptableMethods);
        return {cmd, nil, socks::ErrNoSupportedAuth};
    }

    auto err = socks::ServerAuthUserPass(c, checkUserPass);
    if (err) {
        return {cmd, nil, err};
    }

    AUTO_R(_3, er3, io::ReadFull(c, buf, 3));
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

    // socks::WriteReply(c, socks::ReplySuccess);

    return {cmd, raddr, nil};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// handleTCP ...
extern error handleTCP(net::Conn c, net::Addr raddr);

// handleUDP ...
extern error handleUDP(net::Conn c, net::Addr raddr);

////////////////////////////////////////////////////////////////////////////////////////////////////
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
            return handleUDP(c, raddr->ToNetAddr());
        case socks::CmdBind:
            return errors::New("not support socks command: bind");
        default:
            return errors::New("unknow socks command: %d", cmd);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Server ...
struct Server : public proto::IServer {
    string addr_;
    net::Listener ln_;

    Server(const string& addr) : addr_(addr) {}

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //  Start ...
    error Start() override {
        AUTO_R(ln, err, net::Listen(addr_));
        if (err) {
            LOGS_E(TAG << " Start(" << addr_ << "), err: " << err);
            return err;
        }

        LOGS_D(TAG << " Start(" << addr_ << ")");

        ln_ = ln;
        gx::go([ln = ln_] {
            for (;;) {
                AUTO_R(c, err, ln->Accept());
                if (err) {
                    LOGS_E(TAG << " accept(), err: " << err);
                    break;
                }

                LOGS_V(TAG << " accept() " << c->RemoteAddr());

                gx::go([c = c] { handleConn(c); });
            }
        });

        return nil;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Close ...
    void Close() override {
        if (ln_) {
            ln_->Close();
        }
        LOGS_D(TAG << " Close()");
    }
};
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j) {
    LOGS_V(TAG << " Conf: " << j);

    auto addr = j["listen"];
    if (!addr.is_string() || addr.empty()) {
        return {nil, errors::New("invalid addr")};
    }

    auto s = std::shared_ptr<xx::Server>(new xx::Server(addr));

    return {s, nil};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    // LOGS_V(TAG << " init()");
    proto::Register("s5", New);
    return true;
}();

NAMESPACE_END_S5
