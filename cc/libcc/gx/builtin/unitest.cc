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

// test_slice ...
void test_slice() {
#define PRINT_SLICE(s) std::cout << "len(" #s ")=" << s.size() << ", " #s "=" << s << std::endl

    byte n1 = '\n', n2 = '\r', n3 = ' ';
    int n4 = -2;
    std::cout << "n1=" << n1 << std::endl;
    std::cout << "n2=" << n2 << std::endl;
    std::cout << "n3=" << n3 << std::endl;
    std::cout << "n4=" << n4 << std::endl;

    slice<byte> s1 = make<byte>(0, 100);
    slice<byte> s2 = append(s1, '$', 'b', 'c', 'x', 'y', 'z');
    slice<byte> s3 = {4, 5, '6', '&', '8', '9'};
    // slice<byte> s3 = {'4'};
    PRINT_SLICE(s3);
    slice<byte> s4 = s3(2, 3);
    slice<byte> s5 = append(s4, 'e', 'f');
    slice<byte> s6 = s5;
    copy(s6, s2);

    PRINT_SLICE(s1);
    PRINT_SLICE(s2);
    PRINT_SLICE(s3);
    PRINT_SLICE(s4);
    PRINT_SLICE(s5);
    PRINT_SLICE(s6);
}

}  // namespace unitest
}  // namespace gx
