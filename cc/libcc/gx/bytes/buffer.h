//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/io/io.h"

namespace gx {
namespace bytes {

// The readOp constants describe the last action performed on
// the buffer, so that UnreadRune and UnreadByte can check for
// invalid usage. opReadRuneX constants are chosen such that
// converted to int they correspond to the rune size that was read.
enum class readOp : int8;

// ErrTooLarge is passed to panic if memory cannot be allocated to store data in a buffer.
extern error ErrTooLarge;

// MinRead is the minimum slice size passed to a Read call by
// Buffer.ReadFrom. As long as the Buffer has at least MinRead bytes beyond
// what is required to hold the contents of r, ReadFrom will not grow the
// underlying buffer.
const int MinRead = 512;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A Buffer is a variable-sized buffer of bytes with Read and Write methods.
// The zero value for Buffer is an empty buffer ready to use.
struct Buffer {
    slice<byte> buf;      // contents are the bytes buf[off : len(buf)]
    int off{0};           // read at &buf[off], write at &buf[len(buf)]
    readOp lastRead;  // last read operation, so that Unread* can work correctly.

   private:
    // empty reports whether the unread portion of the buffer is empty.
    bool empty() { return len(buf) <= off; }

   public:
    // Bytes returns a slice of length b.Len() holding the unread portion of the buffer.
    // The slice is valid for use only until the next buffer modification (that is,
    // only until the next call to a method like Read, Write, Reset, or Truncate).
    // The slice aliases the buffer content at least until the next buffer modification,
    // so immediate changes to the slice will affect the result of future reads.
    slice<byte> Bytes() { return buf(off); }

    // String returns the contents of the unread portion of the buffer
    // as a string. If the Buffer is a nil pointer, it returns "<nil>".
    //
    // To build strings more efficiently, see the strings.Builder type.
    string String();

    // Len returns the number of bytes of the unread portion of the buffer;
    // b.Len() == len(b.Bytes()).
    int Len() { return len(buf) - off; }

    // Cap returns the capacity of the buffer's underlying byte slice, that is, the
    // total space allocated for the buffer's data.
    int Cap() { return cap(buf); }

    // Truncate discards all but the first n unread bytes from the buffer
    // but continues to use the same allocated storage.
    // It panics if n is negative or greater than the length of the buffer.
    void Truncate(int n);

    // Reset resets the buffer to be empty,
    // but it retains the underlying storage for use by future writes.
    // Reset is the same as Truncate(0).
    void Reset();

   private:
    // tryGrowByReslice is a inlineable version of grow for the fast-case where the
    // internal buffer only needs to be resliced.
    // It returns the index where bytes should be written and whether it succeeded.
    R<int, bool> tryGrowByReslice(int n);

    // grow grows the buffer to guarantee space for n more bytes.
    // It returns the index where bytes should be written.
    // If the buffer can't grow it will panic with ErrTooLarge.
    int grow(int n);

   public:
    // Grow grows the buffer's capacity, if necessary, to guarantee space for
    // another n bytes. After Grow(n), at least n bytes can be written to the
    // buffer without another allocation.
    // If n is negative, Grow will panic.
    // If the buffer can't grow it will panic with ErrTooLarge.
    void Grow(int n);

    // Write appends the contents of p to the buffer, growing the buffer as
    // needed. The return value n is the length of p; err is always nil. If the
    // buffer becomes too large, Write will panic with ErrTooLarge.
    R<int, error> Write(slice<byte> p);

    // WriteString appends the contents of s to the buffer, growing the buffer as
    // needed. The return value n is the length of s; err is always nil. If the
    // buffer becomes too large, WriteString will panic with ErrTooLarge.
    R<int, error> WriteString(const string& s);

    // WriteByte appends the byte c to the buffer, growing the buffer as needed.
    // The returned error is always nil, but is included to match bufio.Writer's
    // WriteByte. If the buffer becomes too large, WriteByte will panic with
    // ErrTooLarge.
    error WriteByte(byte c);

    // WriteRune appends the UTF-8 encoding of Unicode code point r to the
    // buffer, returning its length and an error, which is always nil but is
    // included to match bufio.Writer's WriteRune. The buffer is grown as needed;
    // if it becomes too large, WriteRune will panic with ErrTooLarge.
    R<int, error> WriteRune(rune r);

    // ReadFrom reads data from r until EOF and appends it to the buffer, growing
    // the buffer as needed. The return value n is the number of bytes read. Any
    // error except io.EOF encountered during the read is also returned. If the
    // buffer becomes too large, ReadFrom will panic with ErrTooLarge.
    R<int64, error> ReadFrom(io::Reader r);

    // WriteTo writes data to w until the buffer is drained or an error occurs.
    // The return value n is the number of bytes written; it always fits into an
    // int, but it is int64 to match the io.WriterTo interface. Any error
    // encountered during the write is also returned.
    R<int64, error> WriteTo(io::Writer w);

    // Read reads the next len(p) bytes from the buffer or until the buffer
    // is drained. The return value n is the number of bytes read. If the
    // buffer has no data to return, err is io.EOF (unless len(p) is zero);
    // otherwise it is nil.
    R<int, error> Read(slice<byte> p);

    // Next returns a slice containing the next n bytes from the buffer,
    // advancing the buffer as if the bytes had been returned by Read.
    // If there are fewer than n bytes in the buffer, Next returns the entire buffer.
    // The slice is only valid until the next call to a read or write method.
    slice<byte> Next(int n);

    // ReadByte reads and returns the next byte from the buffer.
    // If no byte is available, it returns error io.EOF.
    R<byte, error> ReadByte();

    // ReadRune reads and returns the next UTF-8-encoded
    // Unicode code point from the buffer.
    // If no bytes are available, the error returned is io.EOF.
    // If the bytes are an erroneous UTF-8 encoding, it
    // consumes one byte and returns U+FFFD, 1.
    R<rune, int, error> ReadRune();

    // UnreadRune unreads the last rune returned by ReadRune.
    // If the most recent read or write operation on the buffer was
    // not a successful ReadRune, UnreadRune returns an error.  (In this regard
    // it is stricter than UnreadByte, which will unread the last byte
    // from any read operation.)
    error UnreadRune();

    // UnreadByte unreads the last byte returned by the most recent successful
    // read operation that read at least one byte. If a write has happened since
    // the last read, if the last read returned an error, or if the read read zero
    // bytes, UnreadByte returns an error.
    error UnreadByte();

    // ReadBytes reads until the first occurrence of delim in the input,
    // returning a slice containing the data up to and including the delimiter.
    // If ReadBytes encounters an error before finding a delimiter,
    // it returns the data read before the error and the error itself (often io.EOF).
    // ReadBytes returns err != nil if and only if the returned data does not end in
    // delim.
    R<slice<byte>, error> ReadBytes(byte delim);

   private:
    // readSlice is like ReadBytes but returns a reference to internal buffer data.
    R<slice<byte>, error> readSlice(byte delim);

   public:
    // ReadString reads until the first occurrence of delim in the input,
    // returning a string containing the data up to and including the delimiter.
    // If ReadString encounters an error before finding a delimiter,
    // it returns the data read before the error and the error itself (often io.EOF).
    // ReadString returns err != nil if and only if the returned data does not end
    // in delim.
    R<string, error> ReadString(byte delim);
};

// NewBuffer creates and initializes a new Buffer using buf as its
// initial contents. The new Buffer takes ownership of buf, and the
// caller should not use buf after this call. NewBuffer is intended to
// prepare a Buffer to read existing data. It can also be used to set
// the initial size of the internal buffer for writing. To do that,
// buf should have the desired capacity but a length of zero.
//
// In most cases, new(Buffer) (or just declaring a Buffer variable) is
// sufficient to initialize a Buffer.
inline Ref<Buffer> NewBuffer(slice<byte> buf) {
    auto p = NewRef<Buffer>();
    p->buf = buf;
    return p;
}

// NewBufferString creates and initializes a new Buffer using string s as its
// initial contents. It is intended to prepare a buffer to read an existing
// string.
//
// In most cases, new(Buffer) (or just declaring a Buffer variable) is
// sufficient to initialize a Buffer.
inline Ref<Buffer> NewBufferString(const string& s) {
    auto p = NewRef<Buffer>();
    p->buf.assign((byte*)s.data(), s.length());
    return p;
}

}  // namespace bytes
}  // namespace gx
