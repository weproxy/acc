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
    VecPtr<T> ptr_;
    size_t beg_, end_;

    slice() : beg_(0), end_(0), ptr_(nullptr) {}
    explicit slice(size_t len) : beg_(0), end_(len), ptr_(VecPtr<T>(new Vec<T>(len))) {}
    slice(const slice& r) = default;

    // [i] ...
    T& operator[](size_t i) { return ptr_->operator[](beg_ + i); }
    const T& operator[](size_t i) const { return ptr_->operator[](beg_ + i); }

    // Sub ...
    slice Sub(size_t beg, size_t end) const {
        slice r(*this);
        r.beg_ = beg < 0 ? beg_ : beg_ + beg;
        if (r.beg_ > end_) {
            r.beg_ = end_;
        }
        r.end_ = end < 0 ? end_ : beg_ + end;
        if (r.end_ > end_) {
            r.end_ = end_;
        }
        return r;
    }

    // (beg, end)
    slice operator()(size_t beg, size_t end) const { return Sub(beg, end); }

    // bool() ...
    operator bool() const { return !!ptr_; }

    // Len ...
    size_t Len() const { return ptr_ ? end_ - beg_ : 0; }

    // Data ...
    T* Data() { return ptr_ ? ptr_->data() + beg_ : 0; }
    const T* Data() const { return ptr_ ? ptr_->data() + beg_ : 0; }

    // String ...
    string String() const {
        if (!operator bool()) {
            return "<nil>";
        }

        std::ostringstream ss;
        ss << "[";
        for (size_t i = beg_; i < end_; i++) {
            if (i != beg_) {
                ss << " ";
            }
            ss << ptr_->operator[](i);
        }
        ss << "]";

        return ss.str();
    }

    // create_if_null ...
    void create_if_null(size_t len = 0) {
        if (!ptr_) {
            ptr_ = VecPtr<T>(new Vec<T>(len));
        }
    }
};

// bytesli ...
typedef slice<uint8> bytesli;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace xx {
// append ...
template <typename T>
inline void append(slice<T>&) {}

// append ...
template <typename T = uint8, typename V = uint8, typename... Args>
void append(slice<T>& s, V&& v, Args&&... args) {
    s.ptr_->emplace_back(std::forward<V>(v));
    s.end_++;
    append(s, std::forward<Args>(args)...);
}

// append ...
template <typename T = uint8, typename... Args>
void append(slice<T>& s, Args&&... args) {
    append(s, std::forward<Args>(args)...);
}
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// make ...
template <typename T = uint8>
slice<T> make() {
    return slice<T>();
}

// make ...
template <typename T = uint8>
slice<T> make(size_t len) {
    return slice<T>(len);
}

// make ...
template <typename T = uint8, typename... Args>
slice<T> make(T&& t, Args&&... args) {
    if (sizeof...(args) > 0) {
        slice<T> s(0);
        s.ptr_->emplace_back(std::forward<T>(t));
        s.end_++;
        xx::append<T>(s, std::forward<Args>(args)...);
        return s;
    }
    return slice<T>{};
}

// append ...
template <typename T = uint8>
slice<T> append(const slice<T>& l, const slice<T>& r) {
    slice<T> s(l);
    if (r) {
        s.create_if_null();
        s.ptr_->insert(s.ptr_->begin() + s.end_, r.ptr_->begin() + r.beg_, r.ptr_->begin() + r.end_);
        s.end_ += r.Len();
    }
    return s;
}

// append ...
template <typename T = uint8, typename... Args>
slice<T> append(const slice<T>& l, Args&&... args) {
    slice<T> s(l);
    if (sizeof...(args) > 0) {
        s.create_if_null();
        xx::append<T>(s, std::forward<Args>(args)...);
    }
    return s;
}

}  // namespace gx
