//
// weproxy@foxmail.com 2022/10/03
//

#include "bytes.h"
#include "gx/errors/errors.h"
#include "gx/io/io.h"
#include "gx/unicode/utf8/utf8.h"

namespace gx {
namespace bytes {

static error errNegativeRead = errors::New("bytes.Buffer: reader returned negative count from Read");
static error errUnreadByte = errors::New("bytes.Buffer: UnreadByte: previous operation was not a successful read");

static const int maxInt = int(uint(~uint(0)) >> 1);

// smallBufferSize is an initial allocation minimal capacity.
static const int smallBufferSize = 64;

// The readOp constants describe the last action performed on
// the buffer, so that UnreadRune and UnreadByte can check for
// invalid usage. opReadRuneX constants are chosen such that
// converted to int they correspond to the rune size that was read.
enum class readOp : int8 {
    opRead = -1,      // Any other read operation.
    opInvalid = 0,    // Non-read operation.
    opReadRune1 = 1,  // Read rune of size 1.
    opReadRune2 = 2,  // Read rune of size 2.
    opReadRune3 = 3,  // Read rune of size 3.
    opReadRune4 = 4,  // Read rune of size 4.
};

// String returns the contents of the unread portion of the buffer
// as a string. If the Buffer is a nil pointer, it returns "<nil>".
//
// To build strings more efficiently, see the strings.Builder type.
string Buffer::String() {
    auto p = Bytes();
    return string((char*)p.data(), len(p));
}

// Truncate discards all but the first n unread bytes from the buffer
// but continues to use the same allocated storage.
// It panics if n is negative or greater than the length of the buffer.
void Buffer::Truncate(int n) {
    auto& b = *this;
    if (n == 0) {
        b.Reset();
        return;
    }
    b.lastRead = readOp::opInvalid;
    if (n < 0 || n > b.Len()) {
        panic("bytes.Buffer: truncation out of range");
    }
    b.buf = b.buf(0, b.off + n);
}

// Reset resets the buffer to be empty,
// but it retains the underlying storage for use by future writes.
// Reset is the same as Truncate(0).
void Buffer::Reset() {
    auto& b = *this;
    b.buf = b.buf(0);
    b.off = 0;
    b.lastRead = readOp::opInvalid;
}

// tryGrowByReslice is a inlineable version of grow for the fast-case where the
// internal buffer only needs to be resliced.
// It returns the index where bytes should be written and whether it succeeded.
R<int, bool> Buffer::tryGrowByReslice(int n) {
    auto& b = *this;
    int l = len(b.buf);
    if (n <= cap(b.buf) - l) {
        b.buf = b.buf(0, l + n);
        return {l, true};
    }
    return {0, false};
}

// grow grows the buffer to guarantee space for n more bytes.
// It returns the index where bytes should be written.
// If the buffer can't grow it will panic with ErrTooLarge.
int Buffer::grow(int n) {
    auto& b = *this;
    int m = b.Len();
    // If buffer is empty, reset to recover space.
    if (m == 0 && b.off != 0) {
        b.Reset();
    }
    // Try to grow by means of a reslice.
    AUTO_R(i, ok, b.tryGrowByReslice(n));
    if (ok) {
        return i;
    }
    if (!b.buf && n <= smallBufferSize) {
        b.buf = make(n, smallBufferSize);
        return 0;
    }
    int c = cap(b.buf);
    if (n <= c / 2 - m) {
        // We can slide things down instead of allocating a new
        // slice. We only need m+n <= c to slide, but
        // we instead let capacity get twice as large so we
        // don't spend all our time copying.
        copy(b.buf, b.buf(b.off));
    } else if (c > maxInt - c - n) {
        panic(ErrTooLarge);
    } else {
        // Not enough space anywhere, we need to allocate.
        auto buf = make(2 * c + n);
        copy(buf, b.buf(b.off));
        b.buf = buf;
    }
    // Restore b.off and len(b.buf).
    b.off = 0;
    b.buf = b.buf(0, m + n);
    return m;
}

// Grow grows the buffer's capacity, if necessary, to guarantee space for
// another n bytes. After Grow(n), at least n bytes can be written to the
// buffer without another allocation.
// If n is negative, Grow will panic.
// If the buffer can't grow it will panic with ErrTooLarge.
void Buffer::Grow(int n) {
    auto& b = *this;
    if (n < 0) {
        panic("bytes.Buffer.Grow: negative count");
    }
    int m = b.grow(n);
    b.buf = b.buf(0, m);
}

// Write appends the contents of p to the buffer, growing the buffer as
// needed. The return value n is the length of p; err is always nil. If the
// buffer becomes too large, Write will panic with ErrTooLarge.
R<int, error> Buffer::Write(bytez<> p) {
    auto& b = *this;
    b.lastRead = readOp::opInvalid;
    AUTO_R(m, ok, b.tryGrowByReslice(len(p)));
    if (!ok) {
        m = b.grow(len(p));
    }
    return {copy(b.buf(m), p), nil};
}

// WriteString appends the contents of s to the buffer, growing the buffer as
// needed. The return value n is the length of s; err is always nil. If the
// buffer becomes too large, WriteString will panic with ErrTooLarge.
R<int, error> Buffer::WriteString(const string& s) {
    auto& b = *this;
    b.lastRead = readOp::opInvalid;
    AUTO_R(m, ok, b.tryGrowByReslice(len(s)));
    if (!ok) {
        m = b.grow(len(s));
    }
    return {copy(b.buf(m), s), nil};
}

// WriteByte appends the byte c to the buffer, growing the buffer as needed.
// The returned error is always nil, but is included to match bufio.Writer's
// WriteByte. If the buffer becomes too large, WriteByte will panic with
// ErrTooLarge.
error Buffer::WriteByte(byte c) {
    auto& b = *this;
    b.lastRead = readOp::opInvalid;
    AUTO_R(m, ok, b.tryGrowByReslice(1));
    if (!ok) {
        m = b.grow(1);
    }
    b.buf[m] = c;
    return nil;
}

// WriteRune appends the UTF-8 encoding of Unicode code point r to the
// buffer, returning its length and an error, which is always nil but is
// included to match bufio.Writer's WriteRune. The buffer is grown as needed;
// if it becomes too large, WriteRune will panic with ErrTooLarge.
R<int, error> Buffer::WriteRune(rune r) {
    auto& b = *this;
    if (r < utf8::RuneSelf) {
        b.WriteByte(byte(r));
        return {1, nil};
    }
    b.lastRead = readOp::opInvalid;
    AUTO_R(m, ok, b.tryGrowByReslice(utf8::UTFMax));
    if (!ok) {
        m = b.grow(utf8::UTFMax);
    }
    int n = utf8::EncodeRune(b.buf(m, m + utf8::UTFMax), r);
    b.buf = b.buf(0, m + n);
    return {n, nil};
}

// ReadFrom reads data from r until EOF and appends it to the buffer, growing
// the buffer as needed. The return value n is the number of bytes read. Any
// error except io.EOF encountered during the read is also returned. If the
// buffer becomes too large, ReadFrom will panic with ErrTooLarge.
R<int64, error> Buffer::ReadFrom(io::Reader r) {
    auto& b = *this;
    int64 n = 0;
    b.lastRead = readOp::opInvalid;
    for (;;) {
        int i = b.grow(MinRead);
        b.buf = b.buf(0, i);
        AUTO_R(m, e, r->Read(b.buf(i, cap(b.buf))));
        if (m < 0) {
            panic(errNegativeRead);
        }

        b.buf = b.buf(0, i + m);
        n += int64(m);
        if (e == io::ErrEOF) {
            return {n, nil};  // e is EOF, so return nil explicitly
        }
        if (e != nil) {
            return {n, e};
        }
    }
}

// WriteTo writes data to w until the buffer is drained or an error occurs.
// The return value n is the number of bytes written; it always fits into an
// int, but it is int64 to match the io.WriterTo interface. Any error
// encountered during the write is also returned.
R<int64, error> Buffer::WriteTo(io::Writer w) {
    auto& b = *this;
    int64 n = 0;
    b.lastRead = readOp::opInvalid;
    int nBytes = b.Len();
    if (nBytes > 0) {
        AUTO_R(m, e, w->Write(b.buf(b.off)));
        if (m > nBytes) {
            panic("bytes.Buffer.WriteTo: invalid Write count");
        }
        b.off += m;
        n = int64(m);
        if (e != nil) {
            return {n, e};
        }
        // all bytes should have been written, by definition of
        // Write method in io.Writer
        if (m != nBytes) {
            return {n, io::ErrShortWrite};
        }
    }
    // Buffer is now empty; reset.
    b.Reset();
    return {n, nil};
}

// Read reads the next len(p) bytes from the buffer or until the buffer
// is drained. The return value n is the number of bytes read. If the
// buffer has no data to return, err is io.EOF (unless len(p) is zero);
// otherwise it is nil.
R<int, error> Buffer::Read(bytez<> p) {
    auto& b = *this;
    b.lastRead = readOp::opInvalid;
    if (b.empty()) {
        // Buffer is empty, reset to recover space.
        b.Reset();
        if (len(p) == 0) {
            return {0, nil};
        }
        return {0, io::ErrEOF};
    }
    int n = copy(p, b.buf(b.off));
    b.off += n;
    if (n > 0) {
        b.lastRead = readOp::opRead;
    }
    return {n, nil};
}

// Next returns a slice containing the next n bytes from the buffer,
// advancing the buffer as if the bytes had been returned by Read.
// If there are fewer than n bytes in the buffer, Next returns the entire buffer.
// The slice is only valid until the next call to a read or write method.
bytez<> Buffer::Next(int n) {
    auto& b = *this;
    b.lastRead = readOp::opInvalid;
    int m = b.Len();
    if (n > m) {
        n = m;
    }
    auto data = b.buf(b.off, b.off + n);
    b.off += n;
    if (n > 0) {
        b.lastRead = readOp::opRead;
    }
    return data;
}

// ReadByte reads and returns the next byte from the buffer.
// If no byte is available, it returns error io.EOF.
R<byte, error> Buffer::ReadByte() {
    auto& b = *this;
    if (b.empty()) {
        // Buffer is empty, reset to recover space.
        b.Reset();
        return {0, io::ErrEOF};
    }
    byte c = b.buf[b.off];
    b.off++;
    b.lastRead = readOp::opRead;
    return {c, nil};
}

// ReadRune reads and returns the next UTF-8-encoded
// Unicode code point from the buffer.
// If no bytes are available, the error returned is io.EOF.
// If the bytes are an erroneous UTF-8 encoding, it
// consumes one byte and returns U+FFFD, 1.
R<rune, int, error> Buffer::ReadRune() {
    auto& b = *this;
    if (b.empty()) {
        // Buffer is empty, reset to recover space.
        b.Reset();
        return {0, 0, io::ErrEOF};
    }
    byte c = b.buf[b.off];
    if (c < utf8::RuneSelf) {
        b.off++;
        b.lastRead = readOp::opReadRune1;
        return {rune(c), 1, nil};
    }
    AUTO_R(r, n, utf8::DecodeRune(b.buf(b.off)));
    b.off += n;
    b.lastRead = readOp(n);
    return {r, n, nil};
}

// UnreadRune unreads the last rune returned by ReadRune.
// If the most recent read or write operation on the buffer was
// not a successful ReadRune, UnreadRune returns an error.  (In this regard
// it is stricter than UnreadByte, which will unread the last byte
// from any read operation.)
error Buffer::UnreadRune() {
    auto& b = *this;
    if (b.lastRead <= readOp::opInvalid) {
        return errors::New("bytes.Buffer: UnreadRune: previous operation was not a successful ReadRune");
    }
    if (b.off >= int(b.lastRead)) {
        b.off -= int(b.lastRead);
    }
    b.lastRead = readOp::opInvalid;
    return nil;
}
// UnreadByte unreads the last byte returned by the most recent successful
// read operation that read at least one byte. If a write has happened since
// the last read, if the last read returned an error, or if the read read zero
// bytes, UnreadByte returns an error.
error Buffer::UnreadByte() {
    auto& b = *this;
    if (b.lastRead == readOp::opInvalid) {
        return errUnreadByte;
    }
    b.lastRead = readOp::opInvalid;
    if (b.off > 0) {
        b.off--;
    }
    return nil;
}

// ReadBytes reads until the first occurrence of delim in the input,
// returning a slice containing the data up to and including the delimiter.
// If ReadBytes encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often io.EOF).
// ReadBytes returns err != nil if and only if the returned data does not end in
// delim.
R<bytez<>, error> Buffer::ReadBytes(byte delim) {
    auto& b = *this;
    bytez<> line;
    AUTO_R(slice, err, b.readSlice(delim));
    // return a copy of slice. The buffer's backing array may
    // be overwritten by later calls.
    line = append(line, slice);
    return {line, err};
}

// readSlice is like ReadBytes but returns a reference to internal buffer data.
R<bytez<>, error> Buffer::readSlice(byte delim) {
    auto& b = *this;
    bytez<> line;
    error err;

    int i = IndexByte(b.buf(b.off), delim);
    int end = b.off + i + 1;
    if (i < 0) {
        end = len(b.buf);
        err = io::ErrEOF;
    }
    line = b.buf(b.off, end);
    b.off = end;
    b.lastRead = readOp::opRead;
    return {line, err};
}

// ReadString reads until the first occurrence of delim in the input,
// returning a string containing the data up to and including the delimiter.
// If ReadString encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often io.EOF).
// ReadString returns err != nil if and only if the returned data does not end
// in delim.
R<string, error> Buffer::ReadString(byte delim) {
    AUTO_R(slice, err, readSlice(delim));
    return {string(slice), err};
}

}  // namespace bytes
}  // namespace gx
