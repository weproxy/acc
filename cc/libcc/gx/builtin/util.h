//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "def.h"
#include "xx.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// override std::ostream <<
namespace std {
//
inline std::ostream& operator<<(std::ostream& s, unsigned char c) {
    char b[8];
    if (' ' <= c && c <= '~') {
        ::sprintf(b, "'%c'", (char)c);
    } else if (c == '\n') {
        ::sprintf(b, "'\\n'");
    } else if (c == '\r') {
        ::sprintf(b, "'\\r'");
    } else if(c == '\t') {
        ::sprintf(b, "'\\t'");
    } else {
        ::sprintf(b, "%d", c);
    }
    s << b;
    return s;
};

inline std::ostream& operator<<(std::ostream& s, char c) {
    // ...
    return operator<<(s, (unsigned char)c);
};

// if has .String()
// if has .operator string()
// if has ->String()
// if has ->operator string()
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
// tostr ...
template <typename... T>
string tostr(T&&... t) {
    std::ostringstream ss;
    xx::out(ss, std::forward<T>(t)...);
    return ss.str();
}

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

// panic ...
template <typename... T>
void panic(T&&... t) {
    std::ostringstream ss;
    xx::out(ss, std::forward<T>(t)...);
    std::cout << ss.str();
    throw(ss);
}

}  // namespace gx

////////////////////////////////////////////////////////////////////////////////////////////////////
// override std::ostream <<
namespace std {
template <typename T>
std::ostream& operator<<(std::ostream& s, const gx::Vec<T>& t) {
    int i = 0;
    s << "[";
    for (auto& c : t) {
        if (i++) {
            s << ", ";
        }
        s << c;
    }
    s << "]";
    return s;
}

template <typename T>
std::ostream& operator<<(std::ostream& s, const gx::VecPtr<T>& t) {
    s << *t;
    return s;
}

template <typename T>
std::ostream& operator<<(std::ostream& s, const gx::Set<T>& t) {
    int i = 0;
    s << "{";
    for (auto& c : t) {
        if (i++) {
            s << ", ";
        }
        s << c;
    }
    s << "}";
    return s;
}

template <typename T>
std::ostream& operator<<(std::ostream& s, const gx::SetPtr<T>& t) {
    s << *t;
    return s;
}

template <typename T>
std::ostream& operator<<(std::ostream& s, const gx::List<T>& t) {
    int i = 0;
    s << "{";
    for (auto& c : t) {
        if (i++) {
            s << ", ";
        }
        s << c;
    }
    s << "}";
    return s;
}

template <typename T>
std::ostream& operator<<(std::ostream& s, const gx::ListPtr<T>& t) {
    s << *t;
    return s;
}

template <typename K, typename V>
std::ostream& operator<<(std::ostream& s, const gx::Map<K, V>& t) {
    int i = 0;
    s << "{";
    for (auto& c : t) {
        if (i++) {
            s << ", ";
        }
        s << c.first << "=" << c.second;
    }
    s << "}";
    return s;
}

template <typename K, typename V>
std::ostream& operator<<(std::ostream& s, const gx::MapPtr<K, V>& t) {
    s << *t;
    return s;
}
}  // namespace std

namespace gx {
namespace unitest {
void test_defer();
void test_chan();
void test_slice();
}  // namespace unitest
}  // namespace gx
