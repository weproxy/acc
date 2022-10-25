//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "xx.h"

////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
// slice ...
template <typename T = byte>
struct slice {
    int beg_{0}, end_{0};
    VecRef<T> vec_{nullptr};

    slice(const void* p = nil){};
    slice(const slice& r) : beg_(r.beg_), end_(r.end_), vec_(r.vec_) {}
    slice(slice&& r) : beg_(r.beg_), end_(r.end_), vec_(r.vec_) { r._reset(); }
    explicit slice(int len, int cap = 0) : end_(len), vec_(NewRef<Vec<T>>(len)) {
        if (cap > len) {
            vec_->reserve(cap);
        }
    }
    slice(std::initializer_list<T> x) : slice(0, x.size()) {
        for (const auto& e : x) {
            vec_->emplace_back(e);
        }
        end_ += x.size();
    }

    // template <typename X, typename std::enable_if<xx::is_same<X, gx::byte>::value, int>::type = 0>
    // slice<X>(const string& s) : slice<X>(s.length(), s.length()) {
    //     memcpy(data(), s.data(), s.length());
    // }
    slice(const string& s) : slice(s.length(), s.length()) { memcpy(data(), s.data(), s.length()); }

    // operator [i] ...
    T& operator[](int i) { return vec_->operator[](beg_ + i); }
    const T& operator[](int i) const { return vec_->operator[](beg_ + i); }

    // Sub ...
    slice Sub(int beg, int end = -1) const {
        slice r;
        int begt = beg < 0 ? beg_ : (beg_ + beg);
        int endt = end < 0 ? end_ : (beg_ + end);
        if (begt <= endt && endt <= end_) {
            r.beg_ = begt;
            r.end_ = endt;
            r.vec_ = vec_;
        }
        return r;
    }

    // operator (beg, end), likes golang [beg:end]
    slice operator()(int beg, int end = -1) const { return Sub(beg, end); }

    // operator =
    slice& operator=(const slice& r) {
        assign(r);
        return *this;
    }

    // operator =
    slice& operator=(slice&& r) {
        assign(r);
        r._reset();
        return *this;
    }

    // bool() ...
    operator bool() const { return !!vec_; }

    // x == nil or x != nil
    bool operator==(const void* p) const { return p == nil && !!vec_; }
    bool operator!=(const void* p) const { return p == nil && vec_; }

    // size/length ...
    int size() const { return vec_ ? end_ - beg_ : 0; }
    int length() const { return size(); }

    // data ...
    T* data() { return vec_ ? vec_->data() + beg_ : 0; }
    const T* data() const { return vec_ ? vec_->data() + beg_ : 0; }

    // T*() ...
    operator T*() { return data(); }
    operator const T*() const { return data(); }

    // string ...
    operator string() { return string((char*)data(), size()); }
    operator string() const { return string((char*)data(), size()); }

    int len() const { return size(); }
    int cap() const { return vec_ ? vec_->capcity() : 0; }

    // String ...
    string String() const {
        if (!operator bool() || beg_ == end_) {
            return "[]";
        }

        std::ostringstream ss;
        ss << "[";
        for (int i = beg_; i < end_; i++) {
            if (i != beg_) {
                ss << " ";
            }
            ss << vec_->operator[](i);
        }
        ss << "]";

        return ss.str();
    }

   public:
    // _create_if_null ...
    void _create_if_null(int len = 0) {
        if (!vec_) {
            vec_ = NewRef<Vec<T>>(len);
        }
    }

    // assign ...
    void assign(const slice& r) {
        vec_ = r.vec_;
        beg_ = r.beg_;
        end_ = r.end_;
    }

    // assign ...
    void assign(const T* p, int len) {
        _create_if_null(len);
        vec_->insert(vec_->begin(), p, p + len);
        beg_ = 0;
        end_ = len;
    }

    // _reset ...
    void _reset() {
        vec_ = nullptr;
        beg_ = 0;
        end_ = 0;
    }
};

// bytez ...
template <typename T = byte>
using bytez = slice<T>;

// stringz ...
template <typename T = string>
using stringz = slice<T>;

////////////////////////////////////////////////////////////////////////////////
// append ...
namespace xx {
template <typename T>
inline void append(slice<T>&) {}

// append ...
template <typename T = byte, typename V = byte, typename... X>
void append(slice<T>& dst, V&& v, X&&... x) {
    // std::cout << "slice::append(v)" << std::endl;
    auto& vec = dst.vec_;
    auto it = vec->begin() + dst.end_;
    if (it < vec->end()) {
        *it = std::forward<V>(v);
    } else {
        vec->insert(it, std::forward<V>(v));
    }
    dst.end_++;
    append(dst, std::forward<X>(x)...);
}

template <typename T = byte, typename... X>
void append(slice<T>& dst, X&&... x) {
    // std::cout << "slice::append(...)" << std::endl;
    append(dst, std::forward<X>(x)...);
}
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////
//
// make ...
template <typename T = byte>
slice<T> make(int len = 0, int cap = 0) {
    return slice<T>(len, cap);
}

// len ...
inline int len(const string& s) { return s.length(); }

// len ...
template <typename T = byte>
int len(const slice<T>& s) {
    return s.size();
}

// cap ...
template <typename T = byte>
int cap(const slice<T>& s) {
    return s.vec_ ? (s.vec_->capacity() - s.end_) : 0;
}

// append ...
inline bytez<> append(const bytez<>& dst, const string& src) {
    bytez<> s(dst);
    if (!src.empty()) {
        s._create_if_null();
        s.vec_->insert(s.vec_->begin() + s.end_, src.begin(), src.end());
        s.end_ += len(src);
    }
    return s;
}

// append ...
template <typename T = byte, typename... X>
slice<T> append(const slice<T>& dst, const slice<T>& src) {
    slice<T> s(dst);
    if (len(src) > 0) {
        s._create_if_null();
        for (int i = 0; i < len(src); i++) {
            s.vec_->insert(s.vec_->begin() + s.end_ + i, src[i]);
        }
        s.end_ += len(src);
    }
    return s;
}

// append ...
template <typename T = byte, typename V = byte, typename... X>
slice<T> append(const slice<T>& dst, const V& v, X&&... x) {
    slice<T> s(dst);
    s._create_if_null();
    xx::append(s, v);
    if (sizeof...(x) > 0) {
        xx::append<T>(s, std::forward<X>(x)...);
    }
    return s;
}

// copy ...
inline int copy(bytez<>& dst, const string& src) {
    int i = 0;
    for (; i < len(dst) && i < len(src); i++) {
        dst[i] = src[i];
    }
    return i;
}

// copy ...
inline int copy(bytez<>&& dst, const string& src) {
    int i = 0;
    for (; i < len(dst) && i < len(src); i++) {
        dst[i] = src[i];
    }
    return i;
}

// copy ...
template <typename T = byte>
int copy(slice<T>& dst, const slice<T>& src) {
    int i = 0;
    for (; i < len(dst) && i < len(src); i++) {
        dst[i] = src[i];
    }
    return i;
}

// copy ...
template <typename T = byte>
int copy(slice<T>& dst, const void* src, int n) {
    int i = 0;
    const T* r = (const T*)src;
    for (; i < len(dst) && i < n; i++) {
        dst[i] = r[i];
    }
    return i;
}

// copy ...
template <typename T = byte>
int copy(slice<T>&& dst, const slice<T>& src) {
    int i = 0;
    for (; i < len(dst) && i < len(src); i++) {
        dst[i] = src[i];
    }
    return i;
}

// copy ...
template <typename T = byte>
int copy(slice<T>&& dst, const void* src, int n) {
    int i = 0;
    const T* r = (const T*)src;
    for (; i < len(dst) && i < n; i++) {
        dst[i] = r[i];
    }
    return i;
}

}  // namespace gx
