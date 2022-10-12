# WeProxy

```
<Status> developing, not works yet...
```

Cross-platform network proxy programe

A game accelerator on Linux/OSX/Windows, iOS/Android, router hardward



### /cc/

* by C++，use coost, likes golang
* only use <= C++11, compatible with some low version hardware toolchains
* TODO...



| dir       |              |                           |
| ---------- | ------------ | ------------------------- |
| libcc/gx   | likes golang | g = golag                 |
| libcc/fx   | functions       | f = func                  |
| libcc/nx   | network       | n = net                   |
| libcc/logx | logger       |                           |
| libcc/3rd  | third-party     | coost、nlohmann_json... |
|            |              |                           |
|            |              |                           |

| dir         |                |      |
| ------------ | -------------- | ---- |
| app/acc      | client     |      |
| app/acc-serv | server     |      |
| app/acc-turn | control-relay server |      |
|              |                |      |



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



### /go/

* by Golang
* TODO...



### /rs/

* by Rust
* TODO...



