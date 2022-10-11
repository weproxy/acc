//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "xx.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// override std::ostream <<
// if has .String()
// if has .operator string()
// if has ->String()
// if has ->operator string()
namespace std {
template <typename T, typename std::enable_if<gx::xx::is_stringer<T>::value, int>::type = 0>
std::ostream& operator<<(std::ostream& s, T t) {
    s << gx::xx::to_string(t);
    return s;
};
}  // namespace std

// override std::ostream <<
#define GX_DEC_OSTREAM_REF(Obj)                                         \
    inline std::ostream& operator<<(std::ostream& out, const Obj obj) { \
        out << obj.String();                                            \
        return out;                                                     \
    }

// override std::ostream <<
#define GX_DEC_OSTREAM_PTR(Obj)                                         \
    inline std::ostream& operator<<(std::ostream& out, const Obj obj) { \
        out << (obj ? obj->String() : "<nil>");                         \
        return out;                                                     \
    }

#define GX_SS(...)                \
    [&] {                         \
        std::ostringstream _s2s_; \
        _s2s_ << __VA_ARGS__;     \
        return _s2s_.str();       \
    }()

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
// print ...
template <typename... T>
void print(T&&... t) {
    std::ostringstream ss;
    xx::out(ss, std::forward<T>(t)...);
    std::cout << ss.str();
}

// println ...
template <typename... T>
void println(T&&... t) {
    std::ostringstream ss;
    xx::out(ss, std::forward<T>(t)...);
    ss << std::endl;
    std::cout << ss.str();
}
}  // namespace gx
