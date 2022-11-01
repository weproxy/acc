# WeProxy

```
<状态>: 开发中，尚未能工作...
```

一套跨平台网络劫持/代理程序, C++/Go/Rust 编写，运行在 Linux/OSX/Windows、iOS/Android、路由器板子上

一个游戏加速器。



### /cc/

* C++ 版，使用 [coost](https://github.com/idealvin/coost) 协程库及其它
* 仅采用 C++11，尽量兼容各交叉编译 toolchain，因为它们大多仅支持到这一标准
* 用 C++ 模拟 Golang 语法
* TODO...



| 目录       |              |                           |
| ---------- | ------------ | ------------------------- |
| libgx      | 类 golang 库 | g = golag                 |
| libcc/fx   | 功能库       | f = func                  |
| libcc/nx   | 网络库       | n = net                   |
| libcc/logx | 日志库       |                           |
| libcc/3rd  | 第三方库     |                           |
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
