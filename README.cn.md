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


#### code example
* socks5 server
```c++
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
```

```c++

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
```



### /go/

* Golang 版
* TODO...



### /rs/

* Rust 版
* TODO...



