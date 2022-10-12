//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "def.h"
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

namespace std {
// override std::ostream <<
template <typename T>
std::ostream& operator<<(std::ostream& s, const gx::Vec<T>& t) {
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
