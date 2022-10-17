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
typedef std::function<R<int, error>(slice<byte>)> ReadFn;
typedef std::function<R<int, error>(const slice<byte>)> WriteFn;
typedef std::function<error()> CloseFn;

}  // namespace io
}  // namespace gx

////////////////////////////////////////////////////////////////////////////////////////////////////
#include "rw.h"
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace gx {
namespace io {
////////////////////////////////////////////////////////////////////////////////////////////////////
// ...
typedef SharedPtr<xx::reader_t> Reader;
typedef SharedPtr<xx::writer_t> Writer;
typedef SharedPtr<xx::closer_t> Closer;
typedef SharedPtr<xx::readWriter_t> ReadWriter;
typedef SharedPtr<xx::readCloser_t> ReadCloser;
typedef SharedPtr<xx::writeCloser_t> WriteCloser;
typedef SharedPtr<xx::readWriteCloser_t> ReadWriteCloser;

////////////////////////////////////////////////////////////////////////////////
//
// NewReader ...
inline Reader NewReader(const ReadFn& fn) { return SharedPtr<xx::readerFn_t>(new xx::readerFn_t(fn)); }

// NewWriter ...
inline Writer NewWriter(const WriteFn& fn) { return SharedPtr<xx::writerFn_t>(new xx::writerFn_t(fn)); }

// NewCloser ...
inline Closer NewCloser(const CloseFn& fn) { return SharedPtr<xx::closerFn_t>(new xx::closerFn_t(fn)); }

// NewReadWriter ...
inline ReadWriter NewReadWriter(const ReadFn& r, const WriteFn& w) {
    return SharedPtr<xx::readWriterFn_t>(new xx::readWriterFn_t(r, w));
}

// NewReadCloser ...
inline ReadCloser NewReadCloser(const ReadFn& r, const CloseFn& c) {
    return SharedPtr<xx::readCloserFn_t>(new xx::readCloserFn_t(r, c));
}

// NewWriteCloser ...
inline WriteCloser NewWriteCloser(const WriteFn& w, const CloseFn& c) {
    return SharedPtr<xx::writeCloserFn_t>(new xx::writeCloserFn_t(w, c));
}

// NewReadWriteCloser ...
inline ReadWriteCloser NewReadWriteCloser(const ReadFn& r, const WriteFn& w, const CloseFn& c) {
    return SharedPtr<xx::readWriteCloserFn_t>(new xx::readWriteCloserFn_t(r, w, c));
}

////////////////////////////////////////////////////////////////////////////////
//
// NewReader ...
template <typename T, typename std::enable_if<xx::has_read<T>::value, int>::type = 0>
inline Reader NewReader(T t) {
    return SharedPtr<xx::readerObj_t<T>>(new xx::readerObj_t<T>(t));
}

// NewWriter ...
template <typename T, typename std::enable_if<xx::has_write<T>::value, int>::type = 0>
inline Writer NewWriter(T t) {
    return SharedPtr<xx::writerObj_t<T>>(new xx::writerObj_t<T>(t));
}

// NewCloser ...
template <typename T, typename std::enable_if<xx::has_close<T>::value, int>::type = 0>
inline Closer NewCloser(T t) {
    return SharedPtr<xx::closerObj_t<T>>(new xx::closerObj_t<T>(t));
}

// NewReadWriter ...
template <typename T, typename std::enable_if<xx::has_read<T>::value && xx::has_write<T>::value, int>::type = 0>
inline ReadWriter NewReadWriter(T t) {
    return SharedPtr<xx::readWriterObj_t<T>>(new xx::readWriterObj_t<T>(t));
}

// NewReadCloser ...
template <typename T, typename std::enable_if<xx::has_read<T>::value && xx::has_close<T>::value, int>::type = 0>
inline ReadCloser NewReadCloser(T t) {
    return SharedPtr<xx::readCloserObj_t<T>>(new xx::readCloserObj_t<T>(t));
}

// NewWriteCloser ...
template <typename T, typename std::enable_if<xx::has_write<T>::value && xx::has_read<T>::value, int>::type = 0>
inline WriteCloser NewWriteCloser(T t) {
    return SharedPtr<xx::writeCloserObj_t<T>>(new xx::writeCloserObj_t<T>(t));
}

// NewReadWriteCloser ...
template <typename T, typename std::enable_if<
                          xx::has_write<T>::value && xx::has_read<T>::value && xx::has_close<T>::value, int>::type = 0>
inline ReadWriteCloser NewReadWriteCloser(T t) {
    return SharedPtr<xx::readWriteCloserObj_t<T>>(new xx::readWriteCloserObj_t<T>(t));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadAll ..
template <typename Reader, typename std::enable_if<xx::has_read<Reader>::value, int>::type = 0>
ReadCloser NopCloser(Reader r) {
    return SharedPtr<xx::nopCloser_t>(new xx::nopCloser_t());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CopingFn ...
typedef std::function<void(int /*w*/)> CopingFn;

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
extern SharedPtr<xx::discard_t> Discard;

}  // namespace io
}  // namespace gx
