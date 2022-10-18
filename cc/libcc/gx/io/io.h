//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <memory>

#include "gx/errors/errors.h"

namespace gx {
namespace io {
////////////////////////////////////////////////////////////////////////////////////////////////////
// error ...
extern const error ErrShortWrite;
extern const error ErrShortBuffer;
extern const error errInvalidWrite;
extern const error ErrEOF;
extern const error ErrUnexpectedEOF;
extern const error ErrNoProgress;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
using ReadFn = func<R<int, error>(slice<byte>)>;
using WriteFn = func<R<int, error>(const slice<byte>)>;
using CloseFn = func<error()>;

}  // namespace io
}  // namespace gx

////////////////////////////////////////////////////////////////////////////////////////////////////
#include "rw.h"
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace gx {
namespace io {
////////////////////////////////////////////////////////////////////////////////////////////////////
// ...
using Reader = Ref<xx::reader_t>;
using Writer = Ref<xx::writer_t>;
using Closer = Ref<xx::closer_t>;
using ReadWriter = Ref<xx::readWriter_t>;
using ReadCloser = Ref<xx::readCloser_t>;
using WriteCloser = Ref<xx::writeCloser_t>;
using ReadWriteCloser = Ref<xx::readWriteCloser_t>;

////////////////////////////////////////////////////////////////////////////////
//
// NewReaderFn ...
inline Reader NewReaderFn(const ReadFn& fn) { return MakeRef<xx::readerFn_t>(fn); }

// NewWriterFn ...
inline Writer NewWriterFn(const WriteFn& fn) { return MakeRef<xx::writerFn_t>(fn); }

// NewCloserFn ...
inline Closer NewCloserFn(const CloseFn& fn) { return MakeRef<xx::closerFn_t>(fn); }

// NewReadWriterFn ...
inline ReadWriter NewReadWriterFn(const ReadFn& r, const WriteFn& w) {
    return MakeRef<xx::readWriterFn_t>(r, w);
}

// NewReadCloserFn ...
inline ReadCloser NewReadCloserFn(const ReadFn& r, const CloseFn& c) {
    return MakeRef<xx::readCloserFn_t>(r, c);
}

// NewWriteCloserFn ...
inline WriteCloser NewWriteCloserFn(const WriteFn& w, const CloseFn& c) {
    return MakeRef<xx::writeCloserFn_t>(w, c);
}

// NewReadWriteCloserFn ...
inline ReadWriteCloser NewReadWriteCloserFn(const ReadFn& r, const WriteFn& w, const CloseFn& c) {
    return MakeRef<xx::readWriteCloserFn_t>(r, w, c);
}

////////////////////////////////////////////////////////////////////////////////
//
// NewReader ...
template <typename T, typename std::enable_if<xx::has_read<T>::value, int>::type = 0>
inline Reader NewReader(T t) {
    return MakeRef<xx::readerObj_t<T>>(t);
}

// NewWriter ...
template <typename T, typename std::enable_if<xx::has_write<T>::value, int>::type = 0>
inline Writer NewWriter(T t) {
    return MakeRef<xx::writerObj_t<T>>(t);
}

// NewCloser ...
template <typename T, typename std::enable_if<xx::has_close<T>::value, int>::type = 0>
inline Closer NewCloser(T t) {
    return MakeRef<xx::closerObj_t<T>>(t);
}

// NewReadWriter ...
template <typename T, typename std::enable_if<xx::has_read<T>::value && xx::has_write<T>::value, int>::type = 0>
inline ReadWriter NewReadWriter(T t) {
    return MakeRef<xx::readWriterObj_t<T>>(t);
}

// NewReadCloser ...
template <typename T, typename std::enable_if<xx::has_read<T>::value && xx::has_close<T>::value, int>::type = 0>
inline ReadCloser NewReadCloser(T t) {
    return MakeRef<xx::readCloserObj_t<T>>(t);
}

// NewWriteCloser ...
template <typename T, typename std::enable_if<xx::has_write<T>::value && xx::has_read<T>::value, int>::type = 0>
inline WriteCloser NewWriteCloser(T t) {
    return MakeRef<xx::writeCloserObj_t<T>>(t);
}

// NewReadWriteCloser ...
template <typename T, typename std::enable_if<
                          xx::has_write<T>::value && xx::has_read<T>::value && xx::has_close<T>::value, int>::type = 0>
inline ReadWriteCloser NewReadWriteCloser(T t) {
    return MakeRef<xx::readWriteCloserObj_t<T>>(t);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadAll ..
template <typename Reader, typename std::enable_if<xx::has_read<Reader>::value, int>::type = 0>
ReadCloser NopCloser(Reader r) {
    return MakeRef<xx::nopCloser_t>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CopingFn ...
using CopingFn = func<void(int /*w*/)>;

// Copy ...
template <typename Writer, typename Reader,
          typename std::enable_if<xx::has_write<Writer>::value && xx::has_read<Reader>::value, int>::type = 0>
R<size_t /*w*/, error> Copy(Writer w, Reader r, const CopingFn& copingFn = {}) {
    slice<byte> buf = make(1024 * 32);

    size_t written = 0;
    error rerr;

    for (;;) {
        AUTO_R(nr, err, r->Read(buf));
        if (nr > 0) {
            AUTO_R(nw, er2, w->Write(buf(0, nr)));
            if (nw > 0) {
                written += nw;
                if (copingFn) {
                    copingFn(nw);
                }
            }
            if (er2 && !err) {
                err = er2;
            }
        }

        if (err) {
            rerr = err;
            break;
        }
    }

    return {written, rerr};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadAll ..
template <typename Reader, typename std::enable_if<xx::has_read<Reader>::value, int>::type = 0>
R<slice<byte>, error> ReadAll(Reader r) {
    slice<byte> b = make(0, 512);
    for (;;) {
        if (len(b) == cap(b)) {
            // Add more capacity (let append pick how much).
            b = append(b, 0)(0, len(b));
        }
        AUTO_R(n, err, r->Read(b(len(b), cap(b))));
        b = b(0, len(b) + n);
        if (err != nil) {
            if (err == ErrEOF) {
                err = nil;
            }
            return {b, err};
        }
    }
    return {{}, nil};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadFull ..
template <typename Reader, typename std::enable_if<xx::has_read<Reader>::value, int>::type = 0>
R<int, error> ReadFull(Reader r, slice<byte> buf) {
    int nr = 0;
    while (nr < len(buf)) {
        AUTO_R(n, err, r->Read(buf(nr)));
        if (n > 0) {
            nr += n;
        }
        if (err) {
            return {nr, err};
        }
    }
    return {nr, nil};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace xx {
struct discard_t {
    R<int, error> Write(const slice<byte> b) { return {len(b), nil}; }
};
}  // namespace xx

// Discard ...
extern Ref<xx::discard_t> Discard;

}  // namespace io
}  // namespace gx


namespace gx {
namespace unitest {
void test_io();
}  // namespace unitest
}  // namespace gx
