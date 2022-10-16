//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <memory>

#include "gx/builtin/builtin.h"

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
typedef std::function<void()> CloseFn;

}  // namespace io
}  // namespace gx

////////////////////////////////////////////////////////////////////////////////////////////////////
#include "rw.h"
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace gx {
namespace io {
////////////////////////////////////////////////////////////////////////////////////////////////////
// ...
typedef std::shared_ptr<xx::reader_t> Reader;
typedef std::shared_ptr<xx::writer_t> Writer;
typedef std::shared_ptr<xx::closer_t> Closer;
typedef std::shared_ptr<xx::readWriter_t> ReadWriter;
typedef std::shared_ptr<xx::readCloser_t> ReadCloser;
typedef std::shared_ptr<xx::writeCloser_t> WriteCloser;
typedef std::shared_ptr<xx::readWriteCloser_t> ReadWriteCloser;

// NewReader ...
inline Reader NewReader(const ReadFn& fn) { return std::shared_ptr<xx::readerFn_t>(new xx::readerFn_t(fn)); }

// NewWriter ...
inline Writer NewWriter(const WriteFn& fn) { return std::shared_ptr<xx::writerFn_t>(new xx::writerFn_t(fn)); }

// NewCloser ...
inline Closer NewCloser(const CloseFn& fn) { return std::shared_ptr<xx::closerFn_t>(new xx::closerFn_t(fn)); }

// NewReadWriter ...
inline ReadWriter NewReadWriter(const ReadFn& r, const WriteFn& w) {
    return std::shared_ptr<xx::readWriterFn_t>(new xx::readWriterFn_t(r, w));
}

// NewReadCloser ...
inline ReadCloser NewReadCloser(const ReadFn& r, const CloseFn& c) {
    return std::shared_ptr<xx::readCloserFn_t>(new xx::readCloserFn_t(r, c));
}

// NewWriteCloser ...
inline WriteCloser NewWriteCloser(const WriteFn& w, const CloseFn& c) {
    return std::shared_ptr<xx::writeCloserFn_t>(new xx::writeCloserFn_t(w, c));
}

// NewReadWriteCloser ...
inline ReadWriteCloser NewReadWriteCloser(const ReadFn& r, const WriteFn& w, const CloseFn& c) {
    return std::shared_ptr<xx::readWriteCloserFn_t>(new xx::readWriteCloserFn_t(r, w, c));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CopingFn ...
typedef std::function<void(int /*w*/)> CopingFn;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copy ...
template <typename Writer, typename Reader,
          typename std::enable_if<xx::has_write<Writer>::value && xx::has_read<Reader>::value, int>::type = 0>
R<size_t /*w*/, error> Copy(Writer w, Reader r, const CopingFn& copingFn = {}) {
    slice<byte> buf = make(1024 * 32);

    size_t witten = 0;
    error rerr;

    for (;;) {
        AUTO_R(nr, err, r->Read(buf));
        if (nr > 0) {
            AUTO_R(nw, er2, w->Write(buf(0, nr)));
            if (nw > 0) {
                witten += nw;
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

    return {witten, rerr};
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
extern std::shared_ptr<xx::discard_t> Discard;

}  // namespace io
}  // namespace gx
