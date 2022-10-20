//
// weproxy@foxmail.com 2022/10/03
//

#include "htp.h"

#include "gx/io/io.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"

namespace app {
namespace proto {
namespace htp {

////////////////////////////////////////////////////////////////////////////////////////////////////
// handleProxy handle CONNECT command
extern error handleProxy(net::Conn c, slice<byte> hdr);

// handleTunnel handle Tunneling mode
extern error handleTunnel(net::Conn c, slice<byte> hdr);

////////////////////////////////////////////////////////////////////////////////////////////////////
// handleConn ...
static error handleConn(net::Conn c) {
    DEFER(c->Close());

    auto buf = make(1024 * 8);

    c->SetReadDeadline(time::Now().Add(time::Second * 10));

    AUTO_R(n, err, c->Read(buf));
    if (err || n < 8) {
        return err ? err : io::ErrShortBuffer;
    }

    auto dat = buf(0, n);

    if (memcmp(dat.data(), "CONNECT ", 8) == 0) {
        // handle CONNECT command
        return handleTunnel(c, dat);
    }

    // handle reverse-proxy mode
    return handleProxy(c, dat);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// server_t ...
struct server_t : public proto::server_t {
    string addr_;
    net::Listener ln_;

    server_t(const string& addr) : addr_(addr) {}

    ////////////////////////////////////////////////////////////////////////////////////////////////////
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

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Close ...
    virtual error Close() override {
        if (ln_) {
            ln_->Close();
        }
        LOGS_D(TAG << " Close()");
        return nil;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j) {
    LOGS_V(TAG << " Conf: " << j);

    auto addr = j["listen"];
    if (!addr.is_string() || addr.empty()) {
        return {nil, errors::New("invalid addr")};
    }

    return {NewRef<server_t>(string(addr)), nil};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// init ..
static auto _ = [] {
    // LOGS_V(TAG << " init()");
    proto::Register("htp", New);
    return true;
}();

}  // namespace htp
}  // namespace proto
}  // namespace app
