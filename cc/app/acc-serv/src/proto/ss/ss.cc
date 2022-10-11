//
// weproxy@foxmail.com 2022/10/03
//

#include "ss.h"

#include "../proto.h"
#include "logx/logx.h"

NAMESPACE_BEG_SS

namespace xx {

extern error handleTCP(net::Conn c, net::Addr raddr);
extern error handleUDP(net::PacketConn pc, net::Addr caddr, const uint8* buf, size_t len);

////////////////////////////////////////////////////////////////////////////////////////////////////
// handleConn ...
static error handleConn(net::Conn c) {
    DEFER(c->Close());

    return handleTCP(c, nil);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Server ...
struct Server : public proto::IServer {
    std::string addr_;
    net::Listener ln_;
    net::PacketConn pc_;

    Server(const std::string& addr) : addr_(addr) {}

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //  Start ...
    error Start() override {
        AUTO_R(ln, err, net::Listen(addr_));
        if (err) {
            LOGS_E(TAG << " Start(" << addr_ << "), err: " << err);
            return err;
        }

        AUTO_R(pc, er2, net::ListenPacket(addr_));
        if (er2) {
            LOGS_E(TAG << " Start(" << addr_ << "), err: " << er2);
            return er2;
        }

        LOGS_D(TAG << " Start(" << addr_ << ")");

        ln_ = ln;
        pc_ = pc;

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

        gx::go([pc = pc_] {
            uint8* buf = new uint8[1024 * 8];
            DEFER(delete[] buf);

            for (;;) {
                AUTO_R(n, caddr, err, pc->ReadFrom(buf, 1024 * 8));
                if (err) {
                    LOGS_E(TAG << " readFrom(), err: " << err);
                    break;
                }

                if (n > 0) {
                    LOGS_D(TAG << " recvfrom(" << caddr << ") " << n << " bytes");
                    handleUDP(pc, caddr, buf, n);
                }
            }
        });

        return nil;
    }

    // Close ...
    virtual void Close() override {
        if (ln_) {
            ln_->Close();
        }
        if (pc_) {
            pc_->Close();
        }

        LOGS_D(TAG << " Close()");
    }
};

}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
static R<proto::Server, error> New(const json::J& j) {
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
    proto::Register("ss", New);
    return true;
}();

NAMESPACE_END_SS
