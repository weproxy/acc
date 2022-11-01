# WeProxy

```
<Status>: under development, not works yet...
```

A cross platform network capture/proxy program by C++/Go/Rust, run on Linux/OSX/Windows/iOS/Android/Routers.

A game accelerator.


### /cc/

* by C++，use [coost](https://github.com/idealvin/coost) and other libraries
* use <= C++11 only, to compatible with some low version hardware toolchains
* coding c++ like golang
* TODO...



| dir       | func | desc |
| ---------- | ------------ | ------------------------- |
| libgx      |  likes golang | g = golag                 |
| libcc/fx   | functions       | f = func                  |
| libcc/nx   | network       | n = net                   |
| libcc/logx | logger       | log |
| libcc/3rd  | third-party     |                         |
| - |              |                           |

| dir         | func | desc |
| ------------ | -------------- | ---- |
| app/acc      | client     | - |
| app/acc-serv | server     |      |
| app/acc-turn | control-relay server |      |
| - |                |      |



#### build

* use xmake

```shell
cd cc

# app=XXX cmd=YYY make

# build app/acc
make

# build and run app/acc
make run

# build app/acc-serv
cmd=serv make

# build and run app/acc-serv
cmd=serv make run
```



#### A socks5 server codes example

* s5.cc

```c++
//
// weproxy@foxmail.com 2022/10/03
//

#include "s5.h"

#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/socks/socks.h"

namespace internal {
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
        socks::WriteReply(c, socks::ReplyNoAcceptableMethods);
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

    if (socks::CmdConnect != cmd && socks::CmdAssoc != cmd) {
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
extern void handleTCP(net::Conn c, net::Addr raddr);

// handleAssoc ...
extern void handleAssoc(net::Conn c, net::Addr raddr);

////////////////////////////////////////////////////////////////////////////////
// handleConn ...
static void handleConn(net::Conn c) {
    DEFER(c->Close());

    AUTO_R(cmd, raddr, err, handshake(c));
    if (err || !raddr) {
        auto er = err ? err : socks::ErrInvalidAddrType;
        LOGS_E(TAG << " handshake(), err: " << er);
        return;
    }

    switch (cmd) {
        case socks::CmdConnect:
            handleTCP(c, raddr->ToNetAddr());
        case socks::CmdAssoc:
            handleAssoc(c, raddr->ToNetAddr());
        case socks::CmdBind:
            LOGS_E(TAG << " not support socks command: bind");
        default:
            LOGS_E(TAG << " unknow socks command: " << cmd);
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
}  // namespace internal
```

* tcp.cc

```c++
//
// weproxy@foxmail.com 2022/10/03
//

#include "fx/fx.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/netio/netio.h"
#include "nx/socks/socks.h"
#include "nx/stats/stats.h"
#include "s5.h"

namespace internal {
namespace proto {
namespace s5 {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////
// handleTCP ...
void handleTCP(net::Conn c, net::Addr raddr) {
    auto tag = GX_SS(TAG << " TCP_" << nx::NewID() << " " << c->RemoteAddr() << "->" << raddr);
    auto sta = stats::NewTCPStats(stats::TypeS5, tag);

    sta->Start("connected");
    DEFER(sta->Done("closed"));

    // dial target server
    AUTO_R(rc, er1, net::Dial(raddr));
    if (er1) {
        LOGS_E(TAG << " dial, err: " << er1);
        socks::WriteReply(c, socks::ReplyHostUnreachable);
        return;
    }
    DEFER(rc->Close());

    // <<< REP:
    //     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |
    auto err = socks::WriteReply(c, socks::ReplySuccess, 0, net::MakeAddr(net::IPv4zero, 0));
    if (err) {
        if (err != net::ErrClosed) {
            LOGS_E(TAG << " err: " << err);
        }
        return;
    }

    netio::RelayOption opt(time::Second * 2);
    opt.A2B.CopingFn = [sta](int n) { sta->AddRecv(n); };
    opt.B2A.CopingFn = [sta](int n) { sta->AddSent(n); };

    // Relay c <--> rc
    netio::Relay(c, rc, opt);
}

////////////////////////////////////////////////////////////////////////////////
// handleAssoc ...
void handleAssoc(net::Conn c, net::Addr raddr) {
    // TODO ...
}

}  // namespace s5
}  // namespace proto
}  // namespace internal
```



### /go/

* by Golang
* TODO...

#### A socks5 server codes example

* s5.go
```golang
//
// weproxy@foxmail.com 2022/10/20
//

package s5

import (
    "encoding/json"
    "errors"
    "fmt"
    "io"
    "net"
    "time"

    "weproxy/acc/libgo/logx"
    "weproxy/acc/libgo/nx/socks"

    "weproxy/acc/app/acc-serv/internal/proto"
)

// TAG ...
const TAG = "[s5]"

// init ...
func init() {
    proto.Register("s5", New)
}

////////////////////////////////////////////////////////////////////////////////

// New ...
func New(js json.RawMessage) (proto.Server, error) {
    var j struct {
        Listen string `json:"listen,omitempty"`
    }

    if err := json.Unmarshal(js, &j); err != nil {
        return nil, err
    }
    if len(j.Listen) == 0 {
        return nil, errors.New("invalid addr")
    }

    return &server_t{addr: j.Listen}, nil
}

////////////////////////////////////////////////////////////////////////////////

// server_t ...
type server_t struct {
    addr string
    ln   net.Listener
}

// Start ...
func (m *server_t) Start() error {
    ln, err := net.Listen("tcp", m.addr)
    if err != nil {
        logx.E("%v Listen(%v), err: %v", TAG, m.addr, err)
        return err
    }

    logx.D("%v Start(%s)", TAG, m.addr)

    m.ln = ln
    go func() {
        for {
            c, err := ln.Accept()
            if err != nil {
                if !errors.Is(err, net.ErrClosed) {
                    logx.E("%v Accept(), err: %v", TAG, err)
                }
                break
            }

            logx.V("%v Accept() %v", TAG, c.RemoteAddr())

            go handleConn(c)
        }
    }()

    return nil
}

// Close ...
func (m *server_t) Close() error {
    if m.ln != nil {
        m.ln.Close()
    }
    logx.D("%v Close()", TAG)
    return nil
}

////////////////////////////////////////////////////////////////////////////////

// handleConn ...
func handleConn(c net.Conn) {
    defer c.Close()

    cmd, raddr, err := handshake(c)
    if err != nil || raddr == nil {
        logx.E("%v handshake(), err: %v", TAG, err)
        return
    }

    switch cmd {
    case socks.CmdConnect:
        handleTCP(c, raddr.ToTCPAddr())
    case socks.CmdAssoc:
        handleAssoc(c, raddr.ToUDPAddr())
    case socks.CmdBind:
        logx.E("%v not support socks command: bind", TAG)
    default:
        logx.E("%v unknow socks command: %d", TAG, cmd)
    }
}

////////////////////////////////////////////////////////////////////////////////

// checkUserPass ...
func checkUserPass(user, pass string) error {
    return nil
}

// handshake ...
func handshake(c net.Conn) (socks.Command, *socks.Addr, error) {
    c.SetDeadline(time.Now().Add(time.Second * 5))
    defer c.SetDeadline(time.Time{})

    cmd := socks.Command(0)
    buf := make([]byte, 256)

    // >>> REQ:
    //     | VER | NMETHODS | METHODS  |
    //     +-----+----------+----------+
    //     |  1  |    1     | 1 to 255 |

    _, err := io.ReadFull(c, buf[0:2])
    if err != nil {
        socks.WriteReply(c, socks.ReplyAuthFailure, 0, nil)
        return cmd, nil, err
    }

    methodCnt := buf[1]

    if buf[0] != socks.Version5 || methodCnt == 0 || methodCnt > 16 {
        socks.WriteReply(c, socks.ReplyAuthFailure, 0, nil)
        return cmd, nil, socks.ErrInvalidSocksVersion
    }

    n, err := io.ReadFull(c, buf[0:methodCnt])
    if err != nil {
        socks.WriteReply(c, socks.ReplyAuthFailure, 0, nil)
        return cmd, nil, err
    }

    var methodNotRequired, methodUserPass bool
    for i := 0; i < n; i++ {
        switch socks.Method(buf[i]) {
        case socks.AuthMethodNotRequired:
            methodNotRequired = true
        case socks.AuthMethodUserPass:
            methodUserPass = true
        }
    }

    if methodNotRequired || methodUserPass {
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
        err := socks.ServerAuth(c, methodUserPass, checkUserPass)
        if err != nil {
            return cmd, nil, err
        }
    } else {
        socks.WriteReply(c, socks.ReplyNoAcceptableMethods, 0, nil)
        return cmd, nil, socks.ErrNoSupportedAuth
    }

    // >>> REQ:
    //     | VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |

    _, err = io.ReadFull(c, buf[0:3])
    if err != nil {
        socks.WriteReply(c, socks.ReplyAddressNotSupported, 0, nil)
        return cmd, nil, err
    }

    // ver := buf[0]
    cmd = socks.Command(buf[1])
    // rsv := buf[2]

    if socks.CmdConnect != cmd && socks.CmdAssoc != cmd {
        socks.WriteReply(c, socks.ReplyCommandNotSupported, 0, nil)
        return cmd, nil, socks.ToError(socks.ReplyCommandNotSupported)
    }

    _, raddr, err := socks.ReadAddr(c)
    if err != nil {
        socks.WriteReply(c, socks.ReplyAddressNotSupported, 0, nil)
        return cmd, nil, err
    }

    return cmd, raddr, nil
}
```

* tcp.go

```golang
//
// weproxy@foxmail.com 2022/10/20
//

package s5

import (
    "errors"
    "fmt"
    "net"

    "weproxy/acc/libgo/logx"
    "weproxy/acc/libgo/nx"
    "weproxy/acc/libgo/nx/netio"
    "weproxy/acc/libgo/nx/socks"
    "weproxy/acc/libgo/nx/stats"
)

////////////////////////////////////////////////////////////////////////////////

// handleTCP ...
func handleTCP(c net.Conn, raddr net.Addr) {
    tag := fmt.Sprintf("%s TCP_%v %v->%v", TAG, nx.NewID(), c.RemoteAddr(), raddr)
    sta := stats.NewTCPStats(stats.TypeS5, tag)

    sta.Start("connected")
    defer sta.Done("closed")

    // dial target server
    rc, err := net.Dial("tcp", raddr.String())
    if err != nil {
        logx.E("%s dial, err: %v", TAG, err)
        socks.WriteReply(c, socks.ReplyHostUnreachable, 0, nil)
        return
    }
    defer rc.Close()

    // <<< REP:
    //     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |
    err = socks.WriteReply(c, socks.ReplySuccess, 0, &net.TCPAddr{IP: net.IPv4zero, Port: 0})
    if err != nil {
        if !errors.Is(err, net.ErrClosed) {
            logx.E("%s err: ", TAG, err)
        }
        return
    }

    opt := netio.RelayOption{}
    opt.A2B.CopingFn = func(n int) { sta.AddRecv(int64(n)) }
    opt.B2A.CopingFn = func(n int) { sta.AddSent(int64(n)) }

    // Relay c <--> rc
    netio.Relay(c, rc, opt)
}

////////////////////////////////////////////////////////////////////////////////

// handleAssoc ...
func handleAssoc(c net.Conn, raddr net.Addr)  {
    // TODO ...
}
```



### /rs/

* by Rust
* TODO...



