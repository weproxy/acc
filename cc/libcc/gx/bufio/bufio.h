//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/io/io.h"
#include "xx.h"

namespace gx {
namespace bufio {

// Reader ...
template <typename T, typename std::enable_if<gx::io::xx::has_read<T>::value, int>::type = 0>
using Reader = Ref<xx::reader_t<T>>;

// Writer ...
template <typename T, typename std::enable_if<gx::io::xx::has_write<T>::value, int>::type = 0>
using Writer = Ref<xx::writer_t<T>>;

// NewReader ...
template <typename T, typename std::enable_if<gx::io::xx::has_read<T>::value, int>::type = 0>
Reader<T> NewReader(T rd, int size = defaultBufSize) {
    if (size < minReadBufferSize) {
        size = minReadBufferSize;
    }
    auto r = NewRef<xx::reader_t<T>>(rd);
    r->reset(make(size), rd);
    return r;
}

// NewWriter ...
template <typename T, typename std::enable_if<gx::io::xx::has_write<T>::value, int>::type = 0>
Writer<T> NewWriter(T wr, int size = defaultBufSize) {
    if (size < minReadBufferSize) {
        size = minReadBufferSize;
    }
    auto w = NewRef<xx::writer_t<T>>(wr);
    w->buf = make(size);
    return w;
}

}  // namespace bufio
}  // namespace gx
