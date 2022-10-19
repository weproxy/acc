//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "rw.h"

namespace gx {
namespace io {

////////////////////////////////////////////////////////////////////////////////////////////////////
// LimitReader returns a Reader that reads from r
// but stops with EOF after n bytes.
// The underlying implementation is a *LimitedReader.
inline Reader LimitReader(Reader r, int64 n) { return NewRef<LimitedReader>(r, n); }

////////////////////////////////////////////////////////////////////////////////////////////////////
// CopingFn ...
using CopingFn = func<void(int /*w*/)>;

// CopyBuffer is identical to Copy except that it stages through the
// provided buffer (if one is required) rather than allocating a
// temporary one. If buf is nil, one is allocated; otherwise if it has
// zero length, CopyBuffer panics.
//
// If either src implements WriterTo or dst implements ReaderFrom,
// buf will not be used to perform the copy.
template <typename Writer, typename Reader,
          typename std::enable_if<xx::has_write<Writer>::value && xx::has_read<Reader>::value, int>::type = 0>
R<int64 /*w*/, error> CopyBuffer(Writer dst, Reader src, slice<byte> buf, const CopingFn& copingFn = {}) {
    if (!buf || buf.size() == 0) {
        buf = make(0, 32 * 1024);
    }

    int64 written = 0;
    error err;

    for (;;) {
        AUTO_R(nr, er, src->Read(buf));
        if (nr > 0) {
            AUTO_R(nw, ew, dst->Write(buf(0, nr)));
            if (nw < 0 || nr < nw) {
                nw = 0;
                if (ew == nil) {
                    ew = errInvalidWrite;
                }
            }
            written += int64(nw);
            if (copingFn) {
                copingFn(nw);
            }
            if (ew != nil) {
                err = ew;
                break;
            }
            if (nr != nw) {
                err = ErrShortWrite;
                break;
            }
        }
        if (er != nil) {
            if (er != ErrEOF) {
                err = er;
            }
            break;
        }
    }

    return {written, err};
}

// Copy copies from src to dst until either EOF is reached
// on src or an error occurs. It returns the number of bytes
// copied and the first error encountered while copying, if any.
//
// A successful Copy returns err == nil, not err == EOF.
// Because Copy is defined to read from src until EOF, it does
// not treat an EOF from Read as an error to be reported.
//
// If src implements the WriterTo interface,
// the copy is implemented by calling src.WriteTo(dst).
// Otherwise, if dst implements the ReaderFrom interface,
// the copy is implemented by calling dst.ReadFrom(src).
template <typename Writer, typename Reader,
          typename std::enable_if<xx::has_write<Writer>::value && xx::has_read<Reader>::value, int>::type = 0>
R<int64 /*w*/, error> Copy(Writer dst, Reader src, const CopingFn& copingFn = {}) {
    return CopyBuffer(dst, src, make(1024 * 32, 1024 * 32), copingFn);
}

// CopyN copies n bytes (or until an error) from src to dst.
// It returns the number of bytes copied and the earliest
// error encountered while copying.
// On return, written == n if and only if err == nil.
//
// If dst implements the ReaderFrom interface,
// the copy is implemented using it.
template <typename Writer, typename Reader,
          typename std::enable_if<xx::has_write<Writer>::value && xx::has_read<Reader>::value, int>::type = 0>
R<int64 /*w*/, error> CopyN(Writer dst, Reader src, int64 n, const CopingFn& copingFn = {}) {
    int size = std::min<int>(32 * 1024, int(n));
    AUTO_R(written, err, CopyBuffer(dst, LimitReader(src, n), make(size, size), copingFn));
    if (written == n) {
        return {n, nil};
    }
    if (written < n && err == nil) {
        // src stopped early; must have been EOF.
        err = ErrEOF;
    }
    return {written, err};
}

// ReadAll reads from r until an error or EOF and returns the data it read.
// A successful call returns err == nil, not err == EOF. Because ReadAll is
// defined to read from src until EOF, it does not treat an EOF from Read
// as an error to be reported.
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

// ReadAtLeast reads from r into buf until it has read at least min bytes.
// It returns the number of bytes copied and an error if fewer bytes were read.
// The error is EOF only if no bytes were read.
// If an EOF happens after reading fewer than min bytes,
// ReadAtLeast returns ErrUnexpectedEOF.
// If min is greater than the length of buf, ReadAtLeast returns ErrShortBuffer.
// On return, n >= min if and only if err == nil.
// If r returns an error having read at least min bytes, the error is dropped.
template <typename Reader, typename std::enable_if<xx::has_read<Reader>::value, int>::type = 0>
R<int, error> ReadAtLeast(Reader r, slice<byte> buf, int min) {
    if (len(buf) < min) {
        return {0, ErrShortBuffer};
    }
    int n = 0;
    error err;
    while (n < min && err == nil) {
        AUTO_R(nn, er2, r->Read(buf(n)));
        err = er2;
        n += nn;
    }
    if (n >= min) {
        err = nil;
    } else if (n > 0 && err == ErrEOF) {
        err = ErrUnexpectedEOF;
    }
    return {n, err};
}

// ReadFull reads exactly len(buf) bytes from r into buf.
// It returns the number of bytes copied and an error if fewer bytes were read.
// The error is EOF only if no bytes were read.
// If an EOF happens after reading some but not all the bytes,
// ReadFull returns ErrUnexpectedEOF.
// On return, n == len(buf) if and only if err == nil.
// If r returns an error having read at least len(buf) bytes, the error is dropped.
template <typename Reader, typename std::enable_if<xx::has_read<Reader>::value, int>::type = 0>
R<int, error> ReadFull(Reader r, slice<byte> buf) {
    return ReadAtLeast(r, buf, len(buf));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// NopCloser returns a ReadCloser with a no-op Close method wrapping
// the provided Reader r.
// If r implements WriterTo, the returned ReadCloser will implement WriterTo
// by forwarding calls to r.
template <typename Reader, typename std::enable_if<xx::has_read<Reader>::value, int>::type = 0>
ReadCloser NopCloser(Reader r) {
    return NewRef<xx::nopCloser_t>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace xx {
struct discard_t {
    R<int, error> Write(const slice<byte> p) { return {len(p), nil}; }
    R<int, error> WriteString(const string& s) { return {len(s), nil}; }
    R<int, error> ReadFrom(Reader r) {
        int64 n = 0;
        auto b = make(1024 * 8);
        for (;;) {
            AUTO_R(nr, er, r->Read(b));
            n += int64(nr);
            if (er != nil) {
                if (er == ErrEOF) {
                    return {n, nil};
                }
                return {n, er};
            }
        }
        return {n, nil};
    }
};
}  // namespace xx

// Discard is a Writer on which all Write calls succeed
// without doing anything.
extern Ref<xx::discard_t> Discard;

}  // namespace io
}  // namespace gx

namespace gx {
namespace unitest {
void test_io();
}  // namespace unitest
}  // namespace gx
