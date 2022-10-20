//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "def.h"
#include "xx.h"

////////////////////////////////////////////////////////////////////////////////
//
// override std::ostream <<
namespace std {
#if 0
// to_string ...
template <typename T, typename std::enable_if<gx::xx::has_string<T>::value, int>::type = 0>
string to_string(T t) {
    return gx::xx::to_string(t);
}
#endif

// << byte
inline std::ostream& operator<<(std::ostream& s, unsigned char c) {
    char b[8];
    if (' ' <= c && c <= '~') {
        ::sprintf(b, "'%c'", (char)c);
    } else if (c == '\n') {
        ::sprintf(b, "'\\n'");
    } else if (c == '\r') {
        ::sprintf(b, "'\\r'");
    } else if (c == '\t') {
        ::sprintf(b, "'\\t'");
    } else {
        ::sprintf(b, "%d", c);
    }
    s << b;
    return s;
};

// << char
inline std::ostream& operator<<(std::ostream& s, char c) {
    // ...
    return operator<<(s, (unsigned char)c);
};

// if has .String()
// if has .operator string()
// if has ->String()
// if has ->operator string()
template <typename T, typename std::enable_if<gx::xx::has_string<T>::value, int>::type = 0>
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

////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
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
std::ostream& operator<<(std::ostream& s, const gx::VecRef<T>& t) {
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
std::ostream& operator<<(std::ostream& s, const gx::SetRef<T>& t) {
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
std::ostream& operator<<(std::ostream& s, const gx::ListRef<T>& t) {
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
std::ostream& operator<<(std::ostream& s, const gx::MapRef<K, V>& t) {
    s << *t;
    return s;
}
}  // namespace std

////////////////////////////////////////////////////////////////////////////////
//
#ifndef _TCLEAN_
#define _TCLEAN_ "\033[0m"
#define _TWEAK_ "\033[2m"
#define _TULINE_ "\033[4m"

#define _TBLACK_ "\033[30m"
#define _TRED_ "\033[31m"
#define _TGREEN_ "\033[32m"
#define _TYELLOW_ "\033[33m"
#define _TBLUE_ "\033[34m"
#define _TMAGENTA_ "\033[35m"
#define _TCYAN_ "\033[36m"
#define _TWHITE_ "\033[37m"
#endif

// GX_LOG ...
#define GX_LOG_(tag, color, ...)                                                  \
    do {                                                                          \
        std::ostringstream ss;                                                    \
        ss << _TCLEAN_ _TWEAK_ << __FILE__ << ":" << __LINE__ << " " << _TCLEAN_; \
        ss << tag << color << __VA_ARGS__ << _TCLEAN_ << std::endl;               \
        std::cout << ss.str();                                                    \
    } while (0)

#define GX_LOGD(...) GX_LOG_("[D]", _TWHITE_, __VA_ARGS__)
#define GX_LOGI(...) GX_LOG_("[I]", _TGREEN_, __VA_ARGS__)
#define GX_LOGW(...) GX_LOG_("[W]", _TYELLOW_, __VA_ARGS__)
#define GX_LOGE(...) GX_LOG_("[E]", _TRED_, __VA_ARGS__)
#define GX_LOGV(...) GX_LOG_("[V]", _TCYAN_ _TWEAK_, __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
namespace unitest {
void test_defer();
void test_chan();
void test_slice();
}  // namespace unitest
}  // namespace gx
