# WeProxy

```
<状态>: 开发中，尚未能工作...
```

跨平台网络代理程序

游戏加速器，运行在 Linux/OSX/Windows PC、iOS/Android、路由器板子上



### /cc/

* C++ 版，使用 coost 协程库，模拟 Golang
* 仅采用 C++11，尽量兼容各交叉编译 toolchain，因为它们大多仅支持到这一标准
* TODO...



| 目录       |              |                           |
| ---------- | ------------ | ------------------------- |
| libcc/gx   | 类 golang 库 | g = golag                 |
| libcc/fx   | 功能库       | f = func                  |
| libcc/nx   | 网络库       | n = net                   |
| libcc/logx | 日志库       |                           |
| libcc/3rd  | 第三方库     | 主用 coost、nlohmann_json |
|            |              |                           |
|            |              |                           |

| 目录         |                |      |
| ------------ | -------------- | ---- |
| app/acc      | 代理客户端     |      |
| app/acc-serv | 代理服务端     |      |
| app/acc-turn | 中转操控服务端 |      |
|              |                |      |



#### 编译

* 需要 xmake

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

* s5.h
```c++
//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

namespace app {
namespace proto {
namespace s5 {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[s5]";

////////////////////////////////////////////////////////////////////////////////
// New ...
R<proto::Server, error> New(const json::J& j);

}  // namespace s5
}  // namespace proto
}  // namespace app
```

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
    slice<> buf = make(256);

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

namespace app {
namespace proto {
namespace s5 {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////
// handleTCP ...
error handleTCP(net::Conn c, net::Addr raddr) {
    auto tag = GX_SS(TAG << " TCP_" << nx::NewID() << " " << c->RemoteAddr() << "->" << raddr);
    auto sta = stats::NewTCPStats(stats::TypeS5, tag);

    sta->Start("connected");
    DEFER(sta->Done("closed"));

    // dial target server
    AUTO_R(rc, er1, net::Dial(raddr));
    if (er1) {
        LOGS_E(TAG << " dial, err: " << er1);
        socks::WriteReply(c, socks::ReplyHostUnreachable);
        return er1;
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
        return err;
    }

    netio::RelayOption opt(time::Second * 2);
    opt.A2B.CopingFn = [sta](int n) { sta->AddRecv(n); };
    opt.B2A.CopingFn = [sta](int n) { sta->AddSent(n); };

    // Relay c <--> rc
    err = netio::Relay(c, rc, opt);
    if (err) {
        if (err != net::ErrClosed) {
            LOGS_E(TAG << " relay " << tag << " , err: " << err);
        }
    }

    return err;
}

////////////////////////////////////////////////////////////////////////////////
// handleUDP ...
extern error handleUDP(net::PacketConn c, net::Addr caddr, net::Addr raddr);

// handleAssoc ...
error handleAssoc(net::Conn c, net::Addr raddr) {
    auto caddr = c->RemoteAddr();

    auto tag = GX_SS(TAG << " Assoc_" << nx::NewID() << " " << caddr << "->" << raddr);
    auto sta = stats::NewTCPStats(stats::TypeS5, tag);

    sta->Start("connected");
    DEFER(sta->Done("closed"));

    AUTO_R(ln, er1, net::ListenPacket(":0"));
    if (er1) {
        LOGS_E(TAG << " dial, err: " << er1);
        socks::WriteReply(c, socks::ReplyHostUnreachable);
        return er1;
    }

    // handleUDP
    gx::go([ln, caddr, raddr] {
        // raddr is not fixed, it maybe be changed after
        handleUDP(ln, caddr, raddr);
    });

    // <<< REP:
    //     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
    //     +-----+-----+-------+------+----------+----------+
    //     |  1  |  1  | X'00' |  1   |    ...   |    2     |
    auto err = socks::WriteReply(c, socks::ReplySuccess, 0, ln->LocalAddr());
    if (err) {
        if (err != net::ErrClosed) {
            LOGS_E(TAG << " err: " << err);
        }
        return err;
    }

    AUTO_R(_, er2, io::Copy(io::Discard, c));
    if (er2) {
        if (er2 != net::ErrClosed) {
            LOGS_E(TAG << " err: " << er2);
        }
        return er2;
    }

    return nil;
}

}  // namespace s5
}  // namespace proto
}  // namespace app
```

* udp.cc

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

namespace app {
namespace proto {
namespace s5 {
using namespace nx;

////////////////////////////////////////////////////////////////////////////////
// key_t ...
typedef uint64 key_t;

// makeKey ...
static key_t makeKey(net::IP ip, uint16 port) {
    uint64 a = (uint64)(*(uint32*)ip.B.data());  // Only IPv4
    uint64 b = (uint64)(*(uint16*)&port);
    return a << 16 | b;
}
static key_t makeKey(net::Addr addr) { return makeKey(addr->IP, addr->Port); }

////////////////////////////////////////////////////////////////////////////////
// udpSess_t ...
//
// <NAT-Open Notice> maybe one source client send to multi-target server, likes:
//   caddr=1.2.3.4:100 --> raddr=8.8.8.8:53
//                     --> raddr=4.4.4.4:53
//
struct udpSess_t : public std::enable_shared_from_this<udpSess_t> {
    net::PacketConn ln_;  // to source client, it is net::PacketConn
    net::PacketConn rc_;  // to target server, it is udpConn_t
    net::Addr caddr_;     // source client addr
    Ref<stats::Stats> sta_;
    func<void()> afterClosedFn_;

    udpSess_t(net::PacketConn ln, net::PacketConn rc, net::Addr caddr) : ln_(ln), rc_(rc), caddr_(caddr) {}
    ~udpSess_t() { Close(); }

    // Close ...
    error Close() {
        if (rc_) {
            rc_->Close();
            if (afterClosedFn_) {
                afterClosedFn_();
            }
            sta_->Done("closed");
            rc_ = nil;
        }
        return nil;
    }

    // Start ...
    void Start() {
        auto tag = GX_SS(TAG << " UDP_" << nx::NewID() << " " << caddr_);
        auto sta = stats::NewUDPStats(stats::TypeS5, tag);

        sta->Start("started");
        sta_ = sta;

        // loopRecvRC
        auto thiz = shared_from_this();
        gx::go([thiz] {
            DEFER(thiz->Close());

            netio::CopyOption opt(time::Minute * 5, 0);
            opt.WriteAddr = thiz->caddr_;
            opt.CopingFn = [thiz](int size) { thiz->sta_->AddSent(size); };

            // copy to ln_ from rc_
            netio::Copy(thiz->ln_, thiz->rc_, opt);
        });
    }

    // WriteToRC ...
    void WriteToRC(slice<> buf) {
        if (!rc_) {
            return;
        }

        // buf is from source client
        sta_->AddRecv(len(buf));

        // udpConn_t::WriteTo()
        // unpack data (from source client), and writeTo target server
        AUTO_R(n, err, rc_->WriteTo(buf, nil));
        if (err) {
            return;
        }
    }
};

// udpSessMap ...
using udpSessMap = Map<key_t, Ref<udpSess_t>>;

////////////////////////////////////////////////////////////////////////////////
// udpConn_t
// to target server
struct udpConn_t : public net::xx::packetConnWrap_t {
    socks::AddrType typ_{socks::AddrTypeIPv4};

    udpConn_t(net::PacketConn pc) : net::xx::packetConnWrap_t(pc) {}

    // ReadFrom ...
    // readFrom target server, and pack data (to client)
    //
    // <<< RSP:
    //     | RSV | FRAG | ATYP | SRC.ADDR | SRC.PORT | DATA |
    //     +-----+------+------+----------+----------+------+
    //     |  2  |  1   |  1   |    ...   |    2     |  ... |
    virtual R<int, net::Addr, error> ReadFrom(slice<> buf) override {
        if (socks::AddrTypeIPv4 != typ_ && socks::AddrTypeIPv6 != typ_) {
            return {0, nil, socks::ErrInvalidAddrType};
        }

        // readFrom target server
        int pos = 3 + 1 + (socks::AddrTypeIPv4 == typ_ ? 4 : 16) + 2;
        AUTO_R(n, addr, err, wrap_->ReadFrom(buf(pos)));
        if (err) {
            return {n, addr, err};
        }

        // pack data

        auto raddr = socks::FromNetAddr(addr);
        socks::CopyAddr(buf(3), raddr);

        n += 3 + len(raddr->B);

        buf[0] = 0;  // RSV
        buf[1] = 0;  //
        buf[2] = 0;  // FRAG

        return {n, addr, nil};
    }

    // WriteTo ...
    // unpack data (from source client), and writeTo target server
    //
    // >>> REQ:
    //     | RSV | FRAG | ATYP | DST.ADDR | DST.PORT | DATA |
    //     +-----+------+------+----------+----------+------+
    //     |  2  |  1   |  1   |    ...   |    2     |  ... |
    virtual R<int, error> WriteTo(const slice<> buf, net::Addr) override {
        if (len(buf) < 10) {
            return {0, socks::ErrInvalidSocksVersion};
        }

        // unpack data

        AUTO_R(raddr, er1, socks::ParseAddr(buf(3)));
        if (er1) {
            return {0, er1};
        }

        typ_ = socks::AddrType(raddr->B[0]);
        if (socks::AddrTypeIPv4 != typ_ && socks::AddrTypeIPv6 != typ_) {
            return {0, socks::ErrInvalidAddrType};
        }

        // writeTo target server
        int pos = 3 + len(raddr->B);
        AUTO_R(n, er2, wrap_->WriteTo(buf(pos), raddr->ToNetAddr()));
        if (er2) {
            return {n, er2};
        }

        return {n + pos, nil};
    }

    // wrap ...
    static net::PacketConn wrap(net::PacketConn pc) {
        //
        return NewRef<udpConn_t>(pc);
    }
};

////////////////////////////////////////////////////////////////////////////////
// handleUDP ...
//
// <NAT-Open Notice> maybe one source client send to multi-target server, likes:
//   caddr=1.2.3.4:100 --> raddr=8.8.8.8:53
//                     --> raddr=4.4.4.4:53
//
error handleUDP(net::PacketConn ln, net::Addr caddr, net::Addr raddr) {
    // sessMap
    auto sessMap = NewRef<udpSessMap>();
    DEFER({
        for (auto& kv : *sessMap) {
            kv.second->Close();
        }
    });

    DEFER(ln->Close());

    slice<> buf = make(1024 * 8);

    for (;;) {
        // 5 minutes timeout
        ln->SetReadDeadline(time::Now().Add(time::Minute * 5));

        // read
        AUTO_R(n, caddr, err, ln->ReadFrom(buf));
        if (err) {
            if (err != net::ErrClosed) {
                LOGS_E(TAG << " err: " << err);
            }
            break;
        } else if (n <= 0) {
            continue;
        }

        // packet data
        auto data = buf(0, n);

        // lookfor or create sess
        Ref<udpSess_t> sess;

        // use source client addr as key
        // <TODO-Notice>
        //  some user's env has multi-outbound-ip, we will get diff caddr although he use one-same-conn
        key_t key = makeKey(caddr);

        auto it = sessMap->find((key));
        if (it == sessMap->end()) {
            // create a new rc for every new source client
            AUTO_R(c, err, net::ListenPacket(":0"));
            if (err) {
                LOGS_E(TAG << " err: " << err);
                continue;
            }

            // wrap as udpConn_t
            auto rc = udpConn_t::wrap(c);

            // store to sessMap
            sess = NewRef<udpSess_t>(ln, rc, caddr);
            (*sessMap)[key] = sess;

            // remove it after rc closed
            sess->afterClosedFn_ = [sessMap, key] { sessMap->erase(key); };

            // start recv loop...
            sess->Start();
        } else {
            sess = it->second;
        }

        // writeTo target server
        sess->WriteToRC(data);
    }

    return nil;
}

}  // namespace s5
}  // namespace proto
}  // namespace app
```



### /go/

* Golang 版
* TODO...



### /rs/

* Rust 版
* TODO...



