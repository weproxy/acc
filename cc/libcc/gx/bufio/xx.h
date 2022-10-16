//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/builtin/builtin.h"
#include "gx/bytes/bytes.h"
#include "gx/errors/errors.h"
#include "gx/io/io.h"

namespace gx {
namespace bufio {

namespace xx {

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// reader_t ...
template <typename IReader, typename std::enable_if<gx::io::xx::is_reader<IReader>::value, int>::type = 0>
struct reader_t {
    slice<byte> buf;   //
    IReader rd;        // reader provided by the client
    int r{0}, w{0};    // buf read and write positions
    error err;         //
    int lastByte;      // last byte read for UnreadByte; -1 means invalid
    int lastRuneSize;  // size of last rune read for UnreadRune; -1 means invalid

    reader_t(IReader r) : rd(r) {}

    // Size ...
    int Size() { return len(this->buf); }

    // Reset ...
    void Reset(IReader r) {
        if (!this->buf) {
            this->buf = make(defaultBufSize);
        }
        this->reset(buf, r);
    }

    // reset ...
    void reset(slice<byte> buf, IReader r) {
        this->buf = buf;
        this->rd = r;
        this->lastByte = -1;
        this->lastRuneSize = -1;
    }

    // fill ...
    void fill() {
        // Slide existing data to beginning.
        if (this->r > 0) {
            copy(this->buf, this->buf(this->r, this->w));
            this->w -= this->r;
            this->r = 0;
        }

        if (this->w >= len(this->buf)) {
            panic("bufio: tried to fill full buffer");
        }

        // Read new data: try a limited number of times.
        for (int i = maxConsecutiveEmptyReads; i > 0; i--) {
            AUTO_R(n, err, this->rd.Read(this->buf(this->w)));
            if (n < 0) {
                panic(errNegativeRead);
            }
            this->w += n;
            if (err) {
                this->err = err;
                return;
            }
            if (n > 0) {
                return;
            }
        }
        this->err = io::ErrNoProgress;
    }

    // readErr ...
    error readErr() {
        auto err = this->err;
        this->err = nil;
        return err;
    }

    // Peek ...
    R<slice<byte>, error> Peek(int n) {
        if (n < 0) {
            return {{}, ErrNegativeCount};
        }

        this->lastByte = -1;
        this->lastRuneSize = -1;

        for (; this->w - this->r < n && this->w - this->r < len(this->buf) && this->err == nil;) {
            this->fill();  // this->w-this->r < len(this->buf) => buffer is not full
        }

        if (n > len(this->buf)) {
            return {this->buf(this->r, this->w), ErrBufferFull};
        }

        // 0 <= n <= len(this->buf)
        error err;
        if (int avail = this->w - this->r; avail < n) {
            // not enough data in buffer
            n = avail;
            err = this->readErr();
            if (err == nil) {
                err = ErrBufferFull;
            }
        }
        return {this->buf(this->r, this->r + n), err};
    }

    // Discard ...
    R<int, error> Discard(int n) {
        int discarded;
        error err;
        if (n < 0) {
            return {0, ErrNegativeCount};
        }
        if (n == 0) {
            return {discarded, err};
        }

        this->lastByte = -1;
        this->lastRuneSize = -1;

        int remain = n;
        for (;;) {
            int skip = this->Buffered();
            if (skip == 0) {
                this->fill();
                skip = this->Buffered();
            }
            if (skip > remain) {
                skip = remain;
            }
            this->r += skip;
            remain -= skip;
            if (remain == 0) {
                return {n, nil};
            }
            if (this->err != nil) {
                return {n - remain, this->readErr()};
            }
        }

        return {discarded, err};
    }

    // Read ...
    R<int, error> Read(slice<byte> p) {
        int n;
        error err;

        n = len(p);
        if (n == 0) {
            if (this->Buffered() > 0) {
                return {0, nil};
            }
            return {0, this->readErr()};
        }
        if (this->r == this->w) {
            if (this->err != nil) {
                return {0, this->readErr()};
            }
            if (len(p) >= len(this->buf)) {
                // Large read, empty buffer.
                // Read directly into p to avoid copy.
                AUTO_R(_n, _err, this->rd.Read(p));
                n = _n;
                this->err = _err;
                if (n < 0) {
                    panic(errNegativeRead);
                }
                if (n > 0) {
                    this->lastByte = int(p[n - 1]);
                    this->lastRuneSize = -1;
                }
                return {n, this->readErr()};
            }
            // One read.
            // Do not use this->fill, which will loop.
            this->r = 0;
            this->w = 0;
            AUTO_R(_n, _err, this->rd.Read(this->buf));
            n = _n;
            this->err = _err;
            if (n < 0) {
                panic(errNegativeRead);
            }
            if (n == 0) {
                return {0, this->readErr()};
            }
            this->w += n;
        }

        // copy as much as we can
        // Note: if the slice panics here, it is probably because
        // the underlying reader returned a bad count. See issue 49795.
        n = copy(p, this->buf(this->r, this->w));
        this->r += n;
        this->lastByte = int(this->buf[this->r - 1]);
        this->lastRuneSize = -1;
        return {n, nil};
    }

    // ReadByte ...
    R<byte, error> ReadByte() {
        this->lastRuneSize = -1;
        for (; this->r == this->w;) {
            if (this->err != nil) {
                return {0, this->readErr()};
            }
            this->fill();  // buffer is empty
        }
        auto c = this->buf[this->r];
        this->r++;
        this->lastByte = int(c);
        return {c, nil};
    }

    // UnreadByte ...
    error UnreadByte() {
        if (this->lastByte < 0 || this->r == 0 && this->w > 0) {
            return ErrInvalidUnreadByte;
        }
        // this->r > 0 || this->w == 0
        if (this->r > 0) {
            this->r--;
        } else {
            // this->r == 0 && this->w == 0
            this->w = 1;
        }
        this->buf[this->r] = byte(this->lastByte);
        this->lastByte = -1;
        this->lastRuneSize = -1;
        return nil;
    }

    // Buffered ...
    int Buffered() { return this->w - this->r; }

    // ReadSlice ...
    R<slice<byte>, error> ReadSlice(byte delim) {
        slice<byte> line;
        error err;
        int s = 0;  // search start index
        for (;;) {
            // Search buffer.
            if (int i = bytes::IndexByte(this->buf(this->r + s, this->w), delim); i >= 0) {
                i += s;
                line = this->buf(this->r, this->r + i + 1);
                this->r += i + 1;
                break;
            }

            // Pending error?
            if (this->err != nil) {
                line = this->buf(this->r, this->w);
                this->r = this->w;
                err = this->readErr();
                break;
            }

            // Buffer full?
            if (this->Buffered() >= len(this->buf)) {
                this->r = this->w;
                line = this->buf;
                err = ErrBufferFull;
                break;
            }

            s = this->w - this->r;  // do not rescan area we scanned before

            this->fill();  // buffer is not full
        }

        // Handle last byte, if any.
        if (int i = len(line) - 1; i >= 0) {
            this->lastByte = int(line[i]);
            this->lastRuneSize = -1;
        }

        return {line, err};
    }

    // ReadLine ...
    R<slice<byte>, bool, error> ReadLine() {
        bool isPrefix = false;

        AUTO_R(line, err, this->ReadSlice('\n'));
        if (err == ErrBufferFull) {
            // Handle the case where "\r\n" straddles the buffer.
            if (len(line) > 0 && line[len(line) - 1] == '\r') {
                // Put the '\r' back on buf and drop it from line.
                // Let the next call to ReadLine check for "\r\n".
                if (this->r == 0) {
                    // should be unreachable
                    panic("bufio: tried to rewind past start of buffer");
                }
                this->r--;
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
        slice<slice<byte>> fullBuffers;
        slice<byte> finalFragment;
        int totalLen = 0;
        error err;

        slice<byte> frag;
        // Use ReadSlice to look for delim, accumulating full buffers.
        for (;;) {
            AUTO_R(_frag, e, this->ReadSlice(delim));
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
        AUTO_R(full, frag, n, err, this->collectFragments(delim));
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
        // AUTO_R(full, frag, n, err, this->collectFragments(delim));
        // // Allocate new buffer to hold the full pieces and the fragment.
        // var buf strings.Builder;
        // buf.Grow(n);
        // // Copy full pieces and fragment in.
        // for
        //     _, fb : = range full { buf.Write(fb); }
        // buf.Write(frag);
        // return {buf.String(), err};
    }

    // R<int64, error> WriteTo(io::Writer w) {}
    // R<int64, error> writeBuf(io::Writer w) {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// writer_t ...
template <typename IWriter, typename std::enable_if<gx::io::xx::is_writer<IWriter>::value, int>::type = 0>
struct writer_t {
    error err;
    slice<byte> buf;
    int n{0};
    IWriter wr;

    writer_t(IWriter w) : wr(w) {}

    // Size ...
    int Size() { return len(this->buf); }

    // Reset ...
    void Reset(IWriter w) {
        if (this->buf == nil) {
            this->buf = make(defaultBufSize);
        }
        this->err = nil;
        this->n = 0;
        this->wr = w;
    }

    // Flush ...
    error Flush() {
        if (this->err != nil) {
            return this->err;
        }
        if (this->n == 0) {
            return nil;
        }
        AUTO_R(n, _er, this->wr.Write(this->buf(0, this->n)));
        err = _er;
        if (n < this->n && err == nil) {
            err = io::ErrShortWrite;
        }
        if (err != nil) {
            if (n > 0 && n < this->n) {
                copy(this->buf(0, this->n - n), this->buf(n, this->n));
            };
            this->n -= n;
            this->err = err;
            return err;
        }
        this->n = 0;
        return nil;
    }

    // Available ...
    int Available() { return len(this->buf) - this->n; };

    // AvailableBuffer ...
    slice<byte> AvailableBuffer() { return this->buf(this->n)(0, 0); }

    // Buffered ...
    int Buffered() { return this->n; }

    // Write ...
    R<int, error> Write(slice<byte> p) {
        int nn;
        error err;
        for (; len(p) > this->Available() && this->err == nil;) {
            int n = 0;
            if (this->Buffered() == 0) {
                // Large write, empty buffer.
                // Write directly from p to avoid copy.
                AUTO_R(_n, _e, this->wr.Write(p));
                n = _n;
                this->err = _e;
            } else {
                n = copy(this->buf(this->n), p);
                this->n += n;
                this->Flush();
            }
            nn += n;
            p = p(n);
        }
        if (this->err != nil) {
            return {nn, this->err};
        }
        int n = copy(this->buf(this->n), p);
        this->n += n;
        nn += n;
        return {nn, nil};
    }

    // WriteByte ...
    error WriteByte(byte c) {
        if (this->err != nil) {
            return this->err;
        }
        if (this->Available() <= 0 && this->Flush() != nil) {
            return this->err;
        }
        this->buf[this->n] = c;
        this->n++;
        return nil;
    }

    // // WriteString ...
    // R<int, error> WriteString(const string& s) {
    //     io::StringWriter sw;
    //     bool tryStringWriter = true;

    //     int nn == 0;
    //     for (; len(s) > this->Available() && this->err == nil;) {
    //         var n int;
    //         if (this->Buffered() == 0 && sw == nil && tryStringWriter) {
    //             // Check at most once whether this->wr is a StringWriter.
    //             sw, tryStringWriter = this->wr.(io.StringWriter);
    //         }
    //         if (this->Buffered() == 0 && tryStringWriter) {
    //             // Large write, empty buffer, and the underlying writer supports
    //             // WriteString: forward the write to the underlying StringWriter.
    //             // This avoids an extra copy.
    //             AUTO_R(_n, _e, sw.WriteString(s));
    //             n = _n;
    //             this->err = _e;
    //         } else {
    //             n = copy(this->buf(this->n), s);
    //             this->n += n;
    //             this->Flush();
    //         }
    //         nn += n;
    //         s = s(n);
    //     }
    //     if (this->err != nil) {
    //         return {nn, this->err};
    //     }
    //     int n = copy(this->buf(this->n:), s);
    //     this->n += n;
    //     nn += n;
    //     return {nn, nil};
    // }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// readWriter_t ...
template <typename IReader, typename IWriter,
          typename std::enable_if<gx::io::xx::is_reader<IReader>::value && gx::io::xx::is_writer<IReader>::IWriter,
                                  int>::type = 0>
struct readWriter_t {
    IReader rd;
    IWriter wr;
};

}  // namespace xx
}  // namespace bufio
}  // namespace gx
