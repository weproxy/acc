//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <stdint.h>

#include <sstream>

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
// slice ...
template <typename T>
struct slice {
    int beg_{0}, end_{0};
    VecPtr<T> vec_{nullptr};

    explicit slice(int len, int cap = 0) : end_(len), vec_(VecPtr<T>(new Vec<T>(len))) {
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
    slice() = default;
    slice(const slice& r) : beg_(r.beg_), end_(r.end_), vec_(r.vec_) {}
    slice(slice&& r) : beg_(r.beg_), end_(r.end_), vec_(r.vec_) { r._reset(); }

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
    slice& operator=(const slice& r) { _assign(r); }

    // operator =
    slice& operator=(slice&& r) {
        _assign(r);
        r._reset();
        return *this;
    }

    // bool() ...
    operator bool() const { return !!vec_; }

    // size/length ...
    int size() const { return vec_ ? end_ - beg_ : 0; }
    int length() const { return size(); }

    // data ...
    T* data() { return vec_ ? vec_->data() + beg_ : 0; }
    const T* data() const { return vec_ ? vec_->data() + beg_ : 0; }

    // T*() ...
    operator T*() { return data(); }
    operator const T*() const { return data(); }

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
            vec_ = VecPtr<T>(new Vec<T>(len));
        }
    }

    // _assign ...
    void _assign(const slice& r) {
        vec_ = r.vec_;
        beg_ = r.beg_;
        end_ = r.end_;
    }

    // _reset ...
    void _reset() {
        vec_ = nullptr;
        beg_ = 0;
        end_ = 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// append ...
namespace xx {
template <typename T>
inline void append(slice<T>&) {}

template <typename T = byte, typename V = byte, typename... X>
void append(slice<T>& s, V&& v, X&&... x) {
    auto it = s.vec_->begin() + s.end_;
    if (it < s.vec_->end()) {
        *it = std::forward<V>(v);
    } else {
        s.vec_->insert(it, std::forward<V>(v));
    }
    s.end_++;
    append(s, std::forward<X>(x)...);
}

template <typename T = byte, typename... X>
void append(slice<T>& s, X&&... x) {
    append(s, std::forward<X>(x)...);
}
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// make ...
template <typename T = byte>
slice<T> make(int len = 0, int cap = 0) {
    return slice<T>(len, cap);
}

// append ...
template <typename T = byte, typename... X>
slice<T> append(const slice<T>& l, X&&... x) {
    slice<T> s(l);
    if (sizeof...(x) > 0) {
        s._create_if_null();
        xx::append<T>(s, std::forward<X>(x)...);
    }
    return s;
}

// append ...
template <typename T = byte>
slice<T> append(const slice<T>& l, const slice<T>& r) {
    slice<T> s(l);
    if (r) {
        s._create_if_null();
        s.vec_->insert(s.vec_->begin() + s.end_, r.vec_->begin() + r.beg_, r.vec_->begin() + r.end_);
        s.end_ += r.length();
    }
    return s;
}

// copy ...
template <typename T = byte>
int copy(slice<T>& l, const slice<T>& r) {
    int i = 0;
    for (; i < l.size() && i < r.length(); i++) {
        l[i] = r[i];
    }
    return i;
}

// copy ...
template <typename T = byte>
int copy(slice<T>& l, const void* b, int len) {
    int i = 0;
    const T* r = (const T*)b;
    for (; i < l.size() && i < len; i++) {
        l[i] = r[i];
    }
    return i;
}

// copy ...
template <typename T = byte>
int copy(slice<T>&& l, const slice<T>& r) {
    int i = 0;
    for (; i < l.size() && i < r.length(); i++) {
        l[i] = r[i];
    }
    return i;
}

// copy ...
template <typename T = byte>
int copy(slice<T>&& l, const void* b, int len) {
    int i = 0;
    const T* r = (const T*)b;
    for (; i < l.size() && i < len; i++) {
        l[i] = r[i];
    }
    return i;
}

}  // namespace gx
