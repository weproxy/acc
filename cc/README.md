# WeProxy

> use  C++11 only, to compatible with some low version hardware toolchains



### libcc/gx

* C++ library like Golang 

| Golang                                                       | C++                                                          | 说明                             |
| ------------------------------------------------------------ | ------------------------------------------------------------ | -------------------------------- |
| error                                                        | error                                                        | error                            |
| nil                                                          | nil                                                          | nullptr                          |
| func foo() (int, string, error) { <br /> return 0,"s", errors.New(“err”) <br />} | R<int, string, error> foo() { <br />return {0, "s", errors::New("err")};<br /> } | return multi-values              |
| n, s, err := foo()                                           | C++11:   AUTO_R(n, s, err, foo());<br />C++17:   auto [n, s, err] = foo(); | get multi-values                 |
| *T                                                           | Ref\<T\>                                                     | Ret\<T\> == std::shared_ptr\<T\> |
| []T                                                          | slice\<T\>                                                   | slice                            |
| []*T                                                         | slice\<Ref\<T\>\>                                            | slice                            |
| []byte                                                       | bytez\<\>                                                    | byte slice                       |
| B[m:n], B[m:], B[:n], B[:]                                   | B(m, n), B(m), B(0,n), B(0,-1)                               | slice                            |
| len(a), cap(a)                                               | len(a), cap(a)                                               | len, cap                         |
| make([]T, len, cap)                                          | make\<T\>(len, cap)                                          | make                             |
| copy(a, b)                                                   | copy(a, b)                                                   | copy                             |
| append(a, b)                                                 | append(a, b)                                                 | append                           |
| io.Copy(writer, reader)                                      | io::Copy(writer, reader)                                     | io Reader Writer                 |
| defer expr                                                   | DEFER(expr)                                                  | defer                            |
| map[K]V                                                      | map\<K, V\>                                                  | map                              |
| chan T                                                       | chan\<T\>                                                    | chan                             |
| \<More\> ...                                                 | ...                                                          | ...                              |
|                                                              |                                                              |                                  |
| net.Conn, net.PacketConn                                     | net::Conn, net::PacketConn                                   | net lib                          |
| strings.Index                                                | strings::Index                                               | strings lib                      |
| \<More\> ...                                                 | ...                                                          | ...                              |
|                                                              |                                                              |                                  |
| print, println, panic                                        | print, println, panic                                        | func                             |



```golang
// Golang

type CallbackFn func(n int) ([]byte, error)

func foo(fn CallbackFn) (int, string, error) {
  	b, err := fn(100)
    if err != nil {
        return 0, "a", err
    }
  
	  c := append(b[:2], 'c')
  	println("c =", c)
  
  	var wg sync.WaitGroup
	  wg.Add(1)
  
    go func() {
	      defer wg.Done()
      	for {
	          println("hello", 100)
	          time.Sleep(time.Second * 5)
          	break
	      }
    }()
  
	  wg.Wait()
  
  	return 1, "b", errors.New("err")
}
```



```c++
// C++

using CallbackFn = func<R<bytez<>, error>(int)>

R<int, string, error> foo(const CallbackFn& fn)  {
  	// auto [b, err] = fn(100); // >= C++17
  	AUTO_R(b, err, fn(100));    // C++11/14
    if (err != nil) {
        return {0, "a", err};
    }

		auto c = append(b(0, 2), 'c');
  	println("c =", c);
  
  	sync::WaitGroup wg;
  	wg.Add(1);
  
    gx::go([&] {
      	DEFER(wg.Done());
      	for (;;) {
	          println("hello", 100);
	          time::Sleep(time::Second * 5);
          	break;
	      }
    });
  
  	wg.Wait();
  
  	return {1, "b", errors::New("err")};
}
```

