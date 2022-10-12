//
// weproxy@foxmail.com 2022/10/03
//

#include "builtin.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
namespace unitest {
// test_defer ...
void test_defer() {
    Defer defer;

    DEFER(std::cout << "A exit" << std::endl);
    defer([] { std::cout << "A0" << std::endl; });
    defer([] { std::cout << "A1" << std::endl; });

    {
        DEFER(std::cout << "B exit" << std::endl);
        defer([] { std::cout << "B0" << std::endl; });
        defer([] { std::cout << "B1" << std::endl; });
    }

    for (int i = 0; i < 2; i++) {
        DEFER(std::cout << "C" << i << " exit" << std::endl);
        defer([i]() { std::cout << "C" << i << std::endl; });
    }
}

// test_chan ...
void test_chan() {
    auto c = makechan<string>(10);

    gx::go([c] {
        c << "A";
        c << "B";
    });

    string a, b;
    c >> a;
    c >> b;
    std::cout << "a=" << a << ", b=" << b << std::endl;
}

}  // namespace unitest
}  // namespace gx
