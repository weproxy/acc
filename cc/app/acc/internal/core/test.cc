//
// weproxy@foxmail.com 2022/10/03
//

#include "core.h"

#include "fx/signal/signal.h"
#include "gx/fmt/fmt.h"
#include "gx/net/net.h"
#include "internal/conf/conf.h"
#include "internal/proto/proto.h"
#include "internal/server/server.h"
#include "logx/logx.h"

namespace internal {
namespace core {

void test() {
    LOGX_I("[app] cpunum =", os::cpunum());

    gx::go([] {
        const char* host = "www.baidu.com";
        // const char* host = "www.qq.com";
        constexpr int port = 80;

        AUTO_R(c, err, net::Dial(fmt::Sprintf("%s:%d", host, port)));
        if (err) {
            LOGX_E(err);
            return;
        }

        {
            auto s = fmt::Sprintf("GET / HTTP/1.0\r\nHost: %s\r\n\r\n", host);

            AUTO_R(_, err, c->Write(bytez<>(s)));
            if (err) {
                LOGX_E(err);
                return;
            }
        }

        bytez<> buf = make(1024 * 32);

        for (;;) {
            AUTO_R(n, err, c->Read(buf));
            if (n > 0) {
                buf[n] = 0;
                LOGX_D((char*)buf.data());
            }

            if (err) {
                if (err != net::ErrClosed) {
                    LOGX_E(err);
                }
                break;
            }
        }
    });

    gx::go([] {
        DEFER(LOGX_D("s closed"));

        AUTO_R(ln, err, net::Listen("127.0.0.1:11223"));
        if (err) {
            LOGX_E(err);
            return;
        }

        for (;;) {
            AUTO_R(c, err, ln->Accept());
            if (err) {
                LOGX_E(err);
                break;
            }

            gx::go([c]() {
                DEFER(println(c, "closed"));
                io::Copy(c, c);
            });
        }
    });

    gx::go([] {
        for (int i = 0;; i++) {
            LOGX_D(i, "hello");
            time::Sleep(time::Second);
        }
    });
}

}  // namespace core
}  // namespace internal
