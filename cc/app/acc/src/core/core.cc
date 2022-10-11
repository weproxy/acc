//
// weproxy@foxmail.com 2022/10/03
//

#include "core.h"

#include "co/os.h"
#include "conf/conf.h"
#include "gx/os/signal/signal.h"
#include "logx/logx.h"
#include "server/server.h"

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

// Main ...
int Main(int argc, char* argv[]) {
    LOGX_I("[app] cpunum =", os::cpunum());

    flag::init(argc, argv);

    {
        conf::xx::conf_t c;

        c.server.auth.s5 = "s5user:s5pass";
        c.server.auth.ss = "ssuser:sspass";

        c.server.tcp.push_back("s5://1.2.3.4");
        c.server.geo["cn"].push_back("s5://1.2.3.4");
        c.server.geo["us"].push_back("s5://1.2.3.4");

        conf::xx::rule_t r;
        r.host.push_back("a.com");
        r.serv.push_back("default");
        c.rules.push_back(r);

        LOGX_D(&c);

        AUTO_R(cc, err, conf::NewFromJSON(c));
        if (err) {
            LOGX_E(err);
        } else {
            LOGX_W(cc);
        }
    }

    gx::go([]() {
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
            AUTO_R(_, err, c->Write(s.c_str(), s.length()));
            if (err) {
                LOGX_E(err);
                return;
            }
        }

        char* buf = (char*)co::alloc(32 * 1024);
        DEFER(co::free(buf, 32 * 1024));

        for (;;) {
            AUTO_R(n, err, c->Read(buf, 32 * 1024));
            if (n > 0) {
                buf[n] = 0;
                LOGX_D((char*)buf);
            }

            if (err) {
                if (err != net::ErrClosed) {
                    LOGX_E(err);
                }
                break;
            }
        }
    });

    gx::go([]() {
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

    // Wait Ctrl+C or kill -x
    signal::WaitNotify([](int sig) { LOGS_W("[signal] got sig = " << sig); }, SIGINT /*ctrl+c*/, SIGQUIT /*kill -3*/,
                       SIGKILL /*kill -9*/, SIGTERM /*kill -15*/);

    return 0;
}

}  // namespace core
}  // namespace app
