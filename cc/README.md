# WeProxy

> C++ 版，模拟 Golang 语法，使用 C++11 以兼容大多路由器系统的交叉工具



### libcc/gx

| Golang                                                       | C++                                                          | 说明                             |
| ------------------------------------------------------------ | ------------------------------------------------------------ | -------------------------------- |
| error                                                        | error                                                        | error                            |
| nil                                                          | nil                                                          | nullptr                          |
| func foo() (int, string, error) { <br /> return 0,"s", errors.New(“err”) <br />} | R<int, string, error> foo() { <br />return {0, "s", errors::New("err")};<br /> } | return multi-values              |
| n, s, err := foo()                                           | C++11:   AUTO_R(n, s, err, foo());<br />C++17:   auto [n, s, err] = foo(); | get multi-values                 |
| *T                                                           | Ref\<T\>                                                     | Ret\<T\> == std::shared_ptr\<T\> |
| []T                                                          | slice\<T\>                                                   | slice                            |
| []*T                                                         | slice\<Ref\<T\>\>                                            | slice                            |
| []byte                                                       | bytez<>                                                      | byte slice                       |
| len(a)、cap(a)                                               | len(a)、cap(a)                                               | len、cap                         |
| make([]T, len, cap)                                          | make\<T\>(len, cap)                                          | make                             |
| copy(a, b)                                                   | copy(a, b)                                                   | copy                             |
| append(a, b)                                                 | append(a, b)                                                 | append                           |
| io.Copy(writer, reader)                                      | io::Copy(writer, reader)                                     | io Reader Writer                 |
| defer expr                                                   | DEFER(expr)                                                  |                                  |
| map[K]V                                                      | map\<K, V>                                                   | map                              |
| chan T                                                       | chan\<T\>                                                    | chan                             |
| \<More\> ...                                                 | ...                                                          | ...                              |
| net.Conn, net.PacketConn                                     | net::Conn, net::PacketConn                                   | net lib                          |
| strings.Index                                                | strings::Index                                               | strings lib                      |
| \<More\> ...                                                 | ...                                                          | ...                              |

