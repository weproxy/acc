//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/bytes/bytes.h"
#include "gx/fmt/fmt.h"
#include "gx/net/http/http.h"
#include "gx/strings/strings.h"
#include "gx/time/time.h"
#include "htp.h"
#include "logx/logx.h"
#include "nx/netio/netio.h"
#include "nx/stats/stats.h"

namespace app {
namespace proto {
namespace htp {
using namespace nx;

// parseRequest check and return offset/addr
/*
    CONNECT www.qq.com:443 HTTP/1.1
    Host: www.qq.com:443
    User-Agent: curl/7.79.1
    Proxy-Connection: Keep-Alive

*/
static R<int /*off*/, string /*addr*/, error> parseRequest(slice<> hdr) {
    static slice<> CRLF1("\r\n");
    static slice<> CRLF2("\r\n\r\n");

    int off = bytes::Index(hdr, CRLF2);
    if (off < 0) {
        return {0, {}, errors::New("not foud CRLF2")};
    }
    off += len(CRLF2);

    int end = bytes::Index(hdr(0, off), CRLF1);
    if (end < 0) {
        return {off, {}, errors::New("not foud CRLF1'")};
    }

    auto fields = strings::Fields(string(hdr(0, end)));
    if (len(fields) < 3) {
        return {0, {}, errors::New("fields count < 3")};
    }

    if (!strings::EqualFold(fields[0], http::MethodConnect)) {
        LOGS_E(TAG << " handleTunnel() err: ");
        return {0, {}, errors::New("not CONNECT method")};
    }

    AUTO_R(host, port, net::SplitHostPort(fields[1]));
    if (host.empty() || !port) {
        return {0, {}, fmt::Errorf("invalid host")};
    }

    auto addr = fmt::Sprintf("%s:%d", host.c_str(), port);

    return {off, addr, nil};
}

// handleTunnel handle CONNECT command
error handleTunnel(net::Conn c, slice<> hdr) {
    // LOGS_D(TAG << " handleTunnel() req: \n" << string(hdr));

    AUTO_R(off, raddr, err, parseRequest(hdr));
    if (err) {
        LOGS_E(TAG << " handleTunnel() err: " << err);
        return err;
    }

    auto body = hdr(off);
    // if (len(body) > 0) {
    //     LOGS_D(TAG << " handleTunnel() body: \n" << string(body));
    // }

    auto tag = GX_SS(TAG << " TCP_" << nx::NewID() << " " << c->RemoteAddr() << "->" << raddr);
    auto sta = stats::NewTCPStats(stats::TypeHTP, tag);

    sta->Start("connected");
    DEFER(sta->Done("closed"));

    // connect to target
    AUTO_R(rc, er1, net::Dial(raddr));
    if (er1) {
        LOGS_E(TAG << " handleTunnel() err: " << er1);
        static const slice<> FAIL("HTTP/1.1 500\r\n\r\n");
        c->Write(FAIL);
        return er1;
    }
    DEFER(rc->Close());

    // reply 200 OK
    static const slice<> OK("HTTP/1.1 200 Connection Established\r\n\r\n");
    AUTO_R(n, er2, c->Write(OK));
    if (er2) {
        LOGS_E(TAG << " handleTunnel() err: " << er2);
        return er2;
    }
    // sta->AddSent(n);

    netio::RelayOption opt(time::Second * 20);
    opt.A2B.CopingFn = [sta](int n) { sta->AddRecv(n); };
    opt.B2A.CopingFn = [sta](int n) { sta->AddSent(n); };
    if (len(body) > 0) {
        opt.ToB.Data = body;
    }

    // Relay c <--> rc
    err = netio::Relay(c, rc, opt);
    if (err) {
        if (err != net::ErrClosed) {
            LOGS_E(TAG << " relay " << tag << " , err: " << err);
        }
    }

    return err;
}

}  // namespace htp
}  // namespace proto
}  // namespace app
