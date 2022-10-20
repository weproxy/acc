//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/bytes/bytes.h"
#include "gx/errors/errors.h"
#include "gx/gx.h"
#include "gx/io/io.h"

namespace gx {
namespace bufio {

extern const error ErrInvalidUnreadByte;
extern const error ErrInvalidUnreadRune;
extern const error ErrBufferFull;
extern const error ErrNegativeCount;

// defaultBufSize ..
constexpr int defaultBufSize = 4096;
constexpr int minReadBufferSize = 16;
constexpr int maxConsecutiveEmptyReads = 100;

extern const error errNegativeRead;

namespace xx {

////////////////////////////////////////////////////////////////////////////////
//
// reader_t ...
template <typename Reader, typename std::enable_if<gx::io::xx::has_read<Reader>::value, int>::type = 0>
struct reader_t {
    slice<byte> buf;   //
    Reader rd;        // reader provided by the client
    int r{0}, w{0};    // buf read and write positions
    error err;         //
    int lastByte;      // last byte read for UnreadByte; -1 means invalid
    int lastRuneSize;  // size of last rune read for UnreadRune; -1 means invalid

    reader_t(Reader r) : rd(r) {}

    // Size ...
    int Size() { return len(buf); }

    // Reset ...
    void Reset(Reader r) {
        if (!buf) {
            buf = make(defaultBufSize);
        }
        reset(buf, r);
    }

    // reset ...
    void reset(slice<byte> buf, Reader r) {
        auto& b = *this;
        b.buf = buf;
        b.rd = r;
        b.lastByte = -1;
        b.lastRuneSize = -1;
    }

    // fill ...
    void fill() {
        auto& b = *this;
        // Slide existing data to beginning.
        if (b.r > 0) {
            copy(b.buf, b.buf(b.r, b.w));
            b.w -= b.r;
            b.r = 0;
        }

        if (b.w >= len(b.buf)) {
            panic("bufio: tried to fill full buffer");
        }

        // Read new data: try a limited number of times.
        for (int i = maxConsecutiveEmptyReads; i > 0; i--) {
            AUTO_R(n, err, b.rd.Read(b.buf(b.w)));
            if (n < 0) {
                panic(errNegativeRead);
            }
            b.w += n;
            if (err) {
                b.err = err;
                return;
            }
            if (n > 0) {
                return;
            }
        }
        b.err = io::ErrNoProgress;
    }

    // readErr ...
    error readErr() {
        auto& b = *this;
        auto err = b.err;
        b.err = nil;
        return err;
    }

    // Peek ...
    R<slice<byte>, error> Peek(int n) {
        if (n < 0) {
            return {{}, ErrNegativeCount};
        }

        auto& b = *this;

        b.lastByte = -1;
        b.lastRuneSize = -1;

        while (b.w - b.r < n && b.w - b.r < len(b.buf) && b.err == nil) {
            b.fill();  // b.w-b.r < len(b.buf) => buffer is not full
        }

        if (n > len(b.buf)) {
            return {b.buf(b.r, b.w), ErrBufferFull};
        }

        // 0 <= n <= len(b.buf)
        error err;
        int avail = b.w - b.r;
        if (avail < n) {
            // not enough data in buffer
            n = avail;
            err = b.readErr();
            if (err == nil) {
                err = ErrBufferFull;
            }
        }
        return {b.buf(b.r, b.r + n), err};
    }

    // Discard ...
    R<int, error> Discard(int n) {
        auto& b = *this;

        int discarded;
        error err;
        if (n < 0) {
            return {0, ErrNegativeCount};
        }
        if (n == 0) {
            return {discarded, err};
        }

        b.lastByte = -1;
        b.lastRuneSize = -1;

        int remain = n;
        for (;;) {
            int skip = b.Buffered();
            if (skip == 0) {
                b.fill();
                skip = b.Buffered();
            }
            if (skip > remain) {
                skip = remain;
            }
            b.r += skip;
            remain -= skip;
            if (remain == 0) {
                return {n, nil};
            }
            if (b.err != nil) {
                return {n - remain, b.readErr()};
            }
        }

        return {discarded, err};
    }

    // Read ...
    R<int, error> Read(slice<byte> p) {
        auto& b = *this;

        int n;
        error err;

        n = len(p);
        if (n == 0) {
            if (b.Buffered() > 0) {
                return {0, nil};
            }
            return {0, b.readErr()};
        }
        if (b.r == b.w) {
            if (b.err != nil) {
                return {0, b.readErr()};
            }
            if (len(p) >= len(b.buf)) {
                // Large read, empty buffer.
                // Read directly into p to avoid copy.
                AUTO_R(_n, _err, b.rd.Read(p));
                n = _n;
                b.err = _err;
                if (n < 0) {
                    panic(errNegativeRead);
                }
                if (n > 0) {
                    b.lastByte = int(p[n - 1]);
                    b.lastRuneSize = -1;
                }
                return {n, b.readErr()};
            }
            // One read.
            // Do not use b.fill, which will loop.
            b.r = 0;
            b.w = 0;
            AUTO_R(_n, _err, b.rd.Read(b.buf));
            n = _n;
            b.err = _err;
            if (n < 0) {
                panic(errNegativeRead);
            }
            if (n == 0) {
                return {0, b.readErr()};
            }
            b.w += n;
        }

        // copy as much as we can
        // Note: if the slice panics here, it is probably because
        // the underlying reader returned a bad count. See issue 49795.
        n = copy(p, b.buf(b.r, b.w));
        b.r += n;
        b.lastByte = int(b.buf[b.r - 1]);
        b.lastRuneSize = -1;
        return {n, nil};
    }

    // ReadByte ...
    R<byte, error> ReadByte() {
        auto& b = *this;

        b.lastRuneSize = -1;
        while (b.r == b.w) {
            if (b.err != nil) {
                return {0, b.readErr()};
            }
            b.fill();  // buffer is empty
        }
        auto c = b.buf[b.r];
        b.r++;
        b.lastByte = int(c);
        return {c, nil};
    }

    // UnreadByte ...
    error UnreadByte() {
        auto& b = *this;

        if (b.lastByte < 0 || b.r == 0 && b.w > 0) {
            return ErrInvalidUnreadByte;
        }
        // b.r > 0 || b.w == 0
        if (b.r > 0) {
            b.r--;
        } else {
            // b.r == 0 && b.w == 0
            b.w = 1;
        }
        b.buf[b.r] = byte(b.lastByte);
        b.lastByte = -1;
        b.lastRuneSize = -1;
        return nil;
    }

    // Buffered ...
    int Buffered() {
        auto& b = *this;
        return b.w - b.r;
    }

    // ReadSlice ...
    R<slice<byte>, error> ReadSlice(byte delim) {
        auto& b = *this;

        slice<byte> line;
        error err;
        int s = 0;  // search start index
        for (;;) {
            // Search buffer.
            int i = bytes::IndexByte(b.buf(b.r + s, b.w), delim);
            if (i >= 0) {
                i += s;
                line = b.buf(b.r, b.r + i + 1);
                b.r += i + 1;
                break;
            }

            // Pending error?
            if (b.err != nil) {
                line = b.buf(b.r, b.w);
                b.r = b.w;
                err = b.readErr();
                break;
            }

            // Buffer full?
            if (b.Buffered() >= len(b.buf)) {
                b.r = b.w;
                line = b.buf;
                err = ErrBufferFull;
                break;
            }

            s = b.w - b.r;  // do not rescan area we scanned before

            b.fill();  // buffer is not full
        }

        // Handle last byte, if any.
        int i = len(line) - 1;
        if (i >= 0) {
            b.lastByte = int(line[i]);
            b.lastRuneSize = -1;
        }

        return {line, err};
    }

    // ReadLine ...
    R<slice<byte>, bool, error> ReadLine() {
        auto& b = *this;

        bool isPrefix = false;

        AUTO_R(line, err, b.ReadSlice('\n'));
        if (err == ErrBufferFull) {
            // Handle the case where "\r\n" straddles the buffer.
            if (len(line) > 0 && line[len(line) - 1] == '\r') {
                // Put the '\r' back on buf and drop it from line.
                // Let the next call to ReadLine check for "\r\n".
                if (b.r == 0) {
                    // should be unreachable
                    panic("bufio: tried to rewind past start of buffer");
                }
                b.r--;
                line = line(0, len(line) - 1);
            }
            return {line, true, nil};
        }

        if (len(line) == 0) {
            if (err != nil) {
                line = nil;
            }
            return {line, isPrefix, err};
        }
        err = nil;

        if (line[len(line) - 1] == '\n') {
            int drop = 1;
            if (len(line) > 1 && line[len(line) - 2] == '\r') {
                drop = 2;
            }
            line = line(0, len(line) - drop);
        }
        return {line, isPrefix, err};
    }

    // collectFragments ...
    R<slice<slice<byte>>, slice<byte>, int, error> collectFragments(byte delim) {
        auto& b = *this;

        slice<slice<byte>> fullBuffers;
        slice<byte> finalFragment;
        int totalLen = 0;
        error err;

        slice<byte> frag;
        // Use ReadSlice to look for delim, accumulating full buffers.
        for (;;) {
            AUTO_R(_frag, e, b.ReadSlice(delim));
            frag = _frag;
            if (e == nil) {  // got final fragment
                break;
            }
            if (e != ErrBufferFull) {  // unexpected error
                err = e;
                break;
            }

            // Make a copy of the buffer.
            slice<byte> buf = make(len(frag));
            copy(buf, frag);
            fullBuffers = append(fullBuffers, buf);
            totalLen += len(buf);
        }

        totalLen += len(frag);
        return {fullBuffers, frag, totalLen, err};
    }

    // ReadBytes ...
    R<slice<byte>, error> ReadBytes(byte delim) {
        auto& b = *this;

        AUTO_R(full, frag, n, err, b.collectFragments(delim));
        // Allocate new buffer to hold the full pieces and the fragment.
        slice<byte> buf = make(n);
        n = 0;
        // Copy full pieces and fragment in.
        for (int i = 0; i < len(full); i++) {
            n += copy(buf(n), full[i]);
        }
        copy(buf(n), frag);
        return {buf, err};
    }

    // ReadString ...
    R<string, error> ReadString(byte delim) {
        auto& b = *this;

        // AUTO_R(full, frag, n, err, b.collectFragments(delim));
        // // Allocate new buffer to hold the full pieces and the fragment.
        // var buf strings.Builder;
        // buf.Grow(n);
        // // Copy full pieces and fragment in.
        // for (int i = 0; i < len(full); i++) {
        //     buf.Write(full[i]);
        // }
        // buf.Write(frag);
        // return {buf.String(), err};
    }

    // R<int64, error> WriteTo(io::Writer w) {}
    // R<int64, error> writeBuf(io::Writer w) {}
};

////////////////////////////////////////////////////////////////////////////////
//
// writer_t ...
template <typename Writer, typename std::enable_if<gx::io::xx::has_write<Writer>::value, int>::type = 0>
struct writer_t {
    error err;
    slice<byte> buf;
    int n{0};
    Writer wr;

    writer_t(Writer w) : wr(w) {}

    // Size ...
    int Size() { return len(buf); }

    // Reset ...
    void Reset(Writer w) {
        auto& b = *this;

        if (b.buf == nil) {
            b.buf = make(defaultBufSize);
        }
        b.err = nil;
        b.n = 0;
        b.wr = w;
    }

    // Flush ...
    error Flush() {
        auto& b = *this;

        if (b.err != nil) {
            return b.err;
        }
        if (b.n == 0) {
            return nil;
        }
        AUTO_R(n, _er, b.wr.Write(b.buf(0, b.n)));
        err = _er;
        if (n < b.n && err == nil) {
            err = io::ErrShortWrite;
        }
        if (err != nil) {
            if (n > 0 && n < b.n) {
                copy(b.buf(0, b.n - n), b.buf(n, b.n));
            };
            b.n -= n;
            b.err = err;
            return err;
        }
        b.n = 0;
        return nil;
    }

    // Available ...
    int Available() { return len(buf) - n; };

    // AvailableBuffer ...
    slice<byte> AvailableBuffer() { return buf(n)(0, 0); }

    // Buffered ...
    int Buffered() { return n; }

    // Write ...
    R<int, error> Write(slice<byte> p) {
        auto& b = *this;

        int nn;
        error err;
        while (len(p) > b.Available() && b.err == nil) {
            int n = 0;
            if (b.Buffered() == 0) {
                // Large write, empty buffer.
                // Write directly from p to avoid copy.
                AUTO_R(_n, _e, b.wr.Write(p));
                n = _n;
                b.err = _e;
            } else {
                n = copy(b.buf(b.n), p);
                b.n += n;
                b.Flush();
            }
            nn += n;
            p = p(n);
        }
        if (b.err != nil) {
            return {nn, b.err};
        }
        int n = copy(b.buf(b.n), p);
        b.n += n;
        nn += n;
        return {nn, nil};
    }

    // WriteByte ...
    error WriteByte(byte c) {
        auto& b = *this;

        if (b.err != nil) {
            return b.err;
        }
        if (b.Available() <= 0 && b.Flush() != nil) {
            return b.err;
        }
        b.buf[b.n] = c;
        b.n++;
        return nil;
    }

    // // WriteString ...
    // R<int, error> WriteString(const string& s) {
    //     auto& b = *this;
    //     io::StringWriter sw;
    //     bool tryStringWriter = true;

    //     int nn == 0;
    //     while (len(s) > b.Available() && b.err == nil) {
    //         var n int;
    //         if (b.Buffered() == 0 && sw == nil && tryStringWriter) {
    //             // Check at most once whether b.wr is a StringWriter.
    //             sw, tryStringWriter = b.wr.(io.StringWriter);
    //         }
    //         if (b.Buffered() == 0 && tryStringWriter) {
    //             // Large write, empty buffer, and the underlying writer supports
    //             // WriteString: forward the write to the underlying StringWriter.
    //             // This avoids an extra copy.
    //             AUTO_R(_n, _e, sw.WriteString(s));
    //             n = _n;
    //             b.err = _e;
    //         } else {
    //             n = copy(b.buf(b.n), s);
    //             b.n += n;
    //             b.Flush();
    //         }
    //         nn += n;
    //         s = s(n);
    //     }
    //     if (b.err != nil) {
    //         return {nn, b.err};
    //     }
    //     int n = copy(b.buf(b.n:), s);
    //     b.n += n;
    //     nn += n;
    //     return {nn, nil};
    // }
};

////////////////////////////////////////////////////////////////////////////////
//
// readWriter_t ...
template <typename Reader, typename Writer,
          typename std::enable_if<gx::io::xx::has_read<Reader>::value && gx::io::xx::has_write<Reader>::Writer,
                                  int>::type = 0>
struct readWriter_t : public reader_t<Reader>, public writer_t<Writer> {
};

}  // namespace xx
}  // namespace bufio
}  // namespace gx
