//
// weproxy@foxmail.com 2022/10/03
//

#include "core.h"

#include "co/os.h"
#include "gx/fmt/fmt.h"
#include "gx/net/net.h"
#include "gx/os/signal/signal.h"
#include "internal/conf/conf.h"
#include "internal/proto/proto.h"
#include "internal/server/server.h"
#include "logx/logx.h"

namespace app {
namespace core {

// // global ...
// static struct global {
//     std::list<io::Closer> closers_;

//     ~global() { Close(); }

//     void Append(io::Closer c) { closers_.push_front(c); }

//     void Close() {
//         for (auto& c : closers_) {
//             c->Close();
//         }
//         closers_.clear();
//     }
// } _g;

void test() {
    gx::go([] {
        const char* host = "www.baidu.com";
        // const char* host = "www.qq.com";
        const int port = 80;

        AUTO_R(c, err, net::Dial(fmt::Sprintf("%s:%d", host, port)));
        if (err) {
            LOGX_E(err);
            return;
        }

        {
            auto s = fmt::Sprintf("GET / HTTP/1.0\r\nHost: %s\r\n\r\n", host);

            AUTO_R(_, err, c->Write(byte_s(s)));
            if (err) {
                LOGX_E(err);
                return;
            }
        }

        byte_s buf = make(32 * 1024);

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

            gx::go([c = c]() {
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

// Main ...
int Main(int argc, char* argv[]) {
    LOGX_I("[app] cpunum =", os::cpunum());

    flag::init(argc, argv);

    // proto init
    auto err = proto::Init();
    if (err) {
        LOGS_E("[core] proto::Init(), err: " << err);
        proto::Deinit();
        return -1;
    }

    // proto deinit
    DEFER(proto::Deinit());

    test();

    // Wait Ctrl+C or kill -x
    signal::WaitNotify([](int sig) { LOGS_W("[signal] got sig = " << sig); }, SIGINT /*ctrl+c*/, SIGQUIT /*kill -3*/,
                       SIGKILL /*kill -9*/, SIGTERM /*kill -15*/);

    return 0;
}

}  // namespace core
}  // namespace app
