//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <tuple>

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
// R ...
template <typename... T>
using R = std::tuple<T...>;

// GetR ...
template <size_t N, typename T>
auto GetR(T&& t) -> decltype(std::get<N>(t)) {
    return std::get<N>(t);
}

// MakeR ...
template <typename... T>
R<T...> MakeR(T&&... args) {
    return std::make_tuple(std::forward<T>(args)...);
}
}  // namespace gx

////////////////////////////////////////////////////////////////////////////////
// ...
#define _GX_CAT_(x, n) x##n
#define _GX_CAT(x, n) _GX_CAT_(x, n)
#define _GX_EXPAND(x) x

#define _GXR_MICRO(_1, _2, _3, _4, _5, _6, _7, _8, _r, x, ...) x

////////////////////////////////////////////////////////////////////////////////
// for AUTO_R(...)
#if __cplusplus >= 201703L  // >= C++17
#define _GXR_AUTO1(v1, r, x) auto [v1] = r
#define _GXR_AUTO2(v1, v2, r, x) auto [v1, v2] = r
#define _GXR_AUTO3(v1, v2, v3, r, x) auto [v1, v2, v3] = r
#define _GXR_AUTO4(v1, v2, v3, v4, r, x) auto [v1, v2, v3, v4] = r
#define _GXR_AUTO5(v1, v2, v3, v4, v5, r, x) auto [v1, v2, v3, v4, v5] = r
#define _GXR_AUTO6(v1, v2, v3, v4, v5, v6, r, x) auto [v1, v2, v3, v4, v5, v6] = r
#define _GXR_AUTO7(v1, v2, v3, v4, v5, v6, v7, r, x) auto [v1, v2, v3, v4, v5, v6, v7] = r
#define _GXR_AUTO8(v1, v2, v3, v4, v5, v6, v7, v8, r, x) auto [v1, v2, v3, v4, v5, v6, v7, v8] = r
#else  // < C++17
#define _GXR_SET_V(v, n, x) auto v = gx::GetR<n>(x)
#define _GXR_AUTO1(v1, r, x) \
    auto x = r;              \
    _GXR_SET_V(v1, 0, x)
#define _GXR_AUTO2(v1, v2, r, x) \
    _GXR_AUTO1(v1, r, x);        \
    _GXR_SET_V(v2, 1, x)
#define _GXR_AUTO3(v1, v2, v3, r, x) \
    _GXR_AUTO2(v1, v2, r, x);        \
    _GXR_SET_V(v3, 2, x)
#define _GXR_AUTO4(v1, v2, v3, v4, r, x) \
    _GXR_AUTO3(v1, v2, v3, r, x);        \
    _GXR_SET_V(v4, 3, x)
#define _GXR_AUTO5(v1, v2, v3, v4, v5, r, x) \
    _GXR_AUTO4(v1, v2, v3, v4, r, x);        \
    _GXR_SET_V(v5, 4, x)
#define _GXR_AUTO6(v1, v2, v3, v4, v5, v6, r, x) \
    _GXR_AUTO5(v1, v2, v3, v4, v5, r, x);        \
    _GXR_SET_V(v6, 5, x)
#define _GXR_AUTO7(v1, v2, v3, v4, v5, v6, v7, r, x) \
    _GXR_AUTO6(v1, v2, v3, v4, v5, v6, r, x);        \
    _GXR_SET_V(v7, 6, x)
#define _GXR_AUTO8(v1, v2, v3, v4, v5, v6, v7, v8, r, x) \
    _GXR_AUTO7(v1, v2, v3, v4, v5, v6, v7, r, x);        \
    _GXR_SET_V(v8, 7, x)
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define AUTO_R(...)                                                                                            \
    _GX_EXPAND(_GXR_MICRO(__VA_ARGS__, _GXR_AUTO8, _GXR_AUTO7, _GXR_AUTO6, _GXR_AUTO5, _GXR_AUTO4, _GXR_AUTO3, \
                          _GXR_AUTO2, _GXR_AUTO1))                                                             \
    (__VA_ARGS__, _GX_CAT(__x__, __LINE__))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// example
/*
R<int> test1() { return {1}; }
R<int, int> test2() { return {1, 2}; }
R<int, int, int> test3() { return {1, 2, 3}; }
R<int, int, int, int> test4() { return {1, 2, 3, 4}; }
R<int, int, int, int, int> test5() { return {1, 2, 3, 4, 5}; }
R<int, int, int, int, int, int> test6() { return {1, 2, 3, 4, 5, 6}; }
R<int, int, int, int, int, int, int> test7() { return {1, 2, 3, 4, 5, 6, 7}; }
R<int, int, int, int, int, int, int, int> test8() { return {1, 2, 3, 4, 5, 6, 7, 8}; }

void test_AUTO_R() {
    AUTO_R(a1, test1());                              // auto [a1] = test1();
    AUTO_R(b1, b2, test2());                          // auto [b1, b2] = test2();
    AUTO_R(c1, c2, c3, test3());                      // auto [c1, c2, c3] = test3();
    AUTO_R(d1, d2, d3, d4, test4());                  // auto [d1, d2, d3, d4] = test4();
    AUTO_R(e1, e2, e3, e4, e5, test5());              // auto [e1, e2, e3, e4, e5] = test5();
    AUTO_R(f1, f2, f3, f4, f5, f6, test6());          // auto [f1, f2, f3, f4, f5, f6] = test6();
    AUTO_R(g1, g2, g3, g4, g5, g6, g7, test7());      // auto [g1, g2, g3, g4, g5, g6, g7] = test7();
    AUTO_R(h1, h2, h3, h4, h5, h6, h7, h8, test8());  // auto [h1, h2, h3, h4, h5, h6, h7, h8] = test8();

    // if < c++17, equal likes:
    //
    // auto __x__103 = test8();
    // auto h1 = gx::GetR<0>(__x__103);
    // auto h2 = gx::GetR<1>(__x__103);
    // auto h3 = gx::GetR<2>(__x__103);
    // auto h4 = gx::GetR<3>(__x__103);
    // auto h5 = gx::GetR<4>(__x__103);
    // auto h6 = gx::GetR<5>(__x__103);
    // auto h7 = gx::GetR<6>(__x__103);
    // auto h8 = gx::GetR<7>(__x__103);
}
*/
