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



### /go/

* Golang 版
* TODO...



### /rs/

* Rust 版
* TODO...



