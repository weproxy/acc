//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/io/io.h"

namespace gx {
namespace bufio {

extern const error ErrInvalidUnreadByte;
extern const error ErrInvalidUnreadRune;
extern const error ErrBufferFull;
extern const error ErrNegativeCount;

// defaultBufSize ..
const int defaultBufSize = 4096;
const int minReadBufferSize = 16;
const int maxConsecutiveEmptyReads = 100;

extern const error errNegativeRead;

}  // namespace bufio
}  // namespace gx

////////////////////////////////////////////////////////////////////////////////////////////////////
#include "xx.h"
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace gx {
namespace bufio {

// Reader ...
template <typename T, typename std::enable_if<gx::io::xx::has_read<T>::value, int>::type = 0>
using Reader = SharedPtr<xx::reader_t<T>>;

// Writer ...
template <typename T, typename std::enable_if<gx::io::xx::has_write<T>::value, int>::type = 0>
using Writer = SharedPtr<xx::writer_t<T>>;

// NewReader ...
template <typename IReader, typename std::enable_if<gx::io::xx::has_read<IReader>::value, int>::type = 0>
Reader<IReader> NewReader(IReader rd, int size = defaultBufSize) {
    if (size < minReadBufferSize) {
        size = minReadBufferSize;
    }
    Reader<IReader> r(new xx::reader_t<IReader>(rd));
    r->reset(make(size), rd);
    return r;
}

// NewWriter ...
template <typename IWriter, typename std::enable_if<gx::io::xx::has_write<IWriter>::value, int>::type = 0>
Writer<IWriter> NewWriter(IWriter wr, int size = defaultBufSize) {
    if (size < minReadBufferSize) {
        size = minReadBufferSize;
    }
    Writer<IWriter> w(new xx::writer_t<IWriter>(wr));
    w->buf = make(size);
    return w;
}

}  // namespace bufio
}  // namespace gx
