//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "xx.h"

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
//
using ReadByteFn = func<R<byte, error>()>;
using UnreadByteFn = func<error()>;
using WriteByteFn = func<error(byte)>;
using ReadRuneFn = func<R<rune, error>()>;
using UnreadRuneFn = func<error()>;
using WriteStringFn = func<R<int, error>(const string&)>;

namespace xx {

////////////////////////////////////////////////////////////////////////////////////////////////////
// reader_t  ...
struct reader_t {
    virtual ~reader_t() {}
    virtual R<int, error> Read(slice<byte>) = 0;
};

// writer_t  ...
struct writer_t {
    virtual ~writer_t() {}
    virtual R<int, error> Write(const slice<byte>) = 0;
};

// closer_t  ...
struct closer_t {
    virtual ~closer_t() {}
    virtual error Close() = 0;
};

// readWriter_t  ...
struct readWriter_t : public reader_t, public writer_t {};

// readCloser_t  ...
struct readCloser_t : public reader_t, public closer_t {};

// writeCloser_t  ...
struct writeCloser_t : public writer_t, public closer_t {};

// readWriteCloser_t  ...
struct readWriteCloser_t : public reader_t, public writer_t, public closer_t {};

////////////////////////////////////////////////////////////////////////////////////////////////////
// readerFn_t
struct readerFn_t : public reader_t {
    readerFn_t(const ReadFn& r) : r_(r) {}
    virtual R<int, error> Read(slice<byte> b) override { return r_(b); }

   protected:
    ReadFn r_;
};

// writerFn_t
struct writerFn_t : public writer_t {
    writerFn_t(const WriteFn& w) : w_(w) {}
    virtual R<int, error> Write(const slice<byte> b) override { return w_(b); }

   protected:
    WriteFn w_;
};

// closerFn_t
struct closerFn_t : public closer_t {
    closerFn_t(const CloseFn& c) : c_(c) {}
    virtual error Close() override { return c_ ? c_() : nil; }

   protected:
    CloseFn c_;
};

// readWriterFn_t
struct readWriterFn_t : public readWriter_t {
    readWriterFn_t(const ReadFn& r, const WriteFn& w) : r_(r), w_(w) {}
    virtual R<int, error> Read(slice<byte> b) override { return r_(b); }
    virtual R<int, error> Write(const slice<byte> b) override { return w_(b); }

   protected:
    ReadFn r_;
    WriteFn w_;
};

// readCloserFn_t
struct readCloserFn_t : public readCloser_t {
    readCloserFn_t(const ReadFn& r, const CloseFn& c) : r_(r), c_(c) {}
    virtual R<int, error> Read(slice<byte> b) override { return r_(b); }
    virtual error Close() override { return c_(); }

   protected:
    ReadFn r_;
    CloseFn c_;
};

// writeCloserFn_t
struct writeCloserFn_t : public writeCloser_t {
    writeCloserFn_t(const WriteFn& w, const CloseFn& c) : w_(w), c_(c) {}
    virtual R<int, error> Write(const slice<byte> b) override { return w_(b); }
    virtual error Close() override { return c_(); }

   protected:
    WriteFn w_;
    CloseFn c_;
};

// readWriteCloserFn_t
struct readWriteCloserFn_t : public readWriteCloser_t {
    readWriteCloserFn_t(const ReadFn& r, const WriteFn& w, const CloseFn& c) : r_(r), w_(w), c_(c) {}
    virtual R<int, error> Read(slice<byte> b) override { return r_(b); }
    virtual R<int, error> Write(const slice<byte> b) override { return w_(b); }
    virtual error Close() override { return c_ ? c_() : nil; }

   protected:
    ReadFn r_;
    WriteFn w_;
    CloseFn c_;
};

////////////////////////////////////////////////////////////////////////////////
// byteReader_t ...
struct byteReader_t {
    virtual R<byte, error> ReadByte() = 0;
};

// byteWriter_t ...
struct byteWriter_t {
    virtual error WriteByte(byte c) = 0;
};

// byteScanner_t ...
struct byteScanner_t : public byteReader_t {
    virtual error UnreadByte() = 0;
};

// runeReader_t ...
struct runeReader_t {
    virtual R<rune, error> ReadRune() = 0;
};

// runeScanner_t ...
struct runeScanner_t : public runeReader_t {
    virtual error UnreadRune() = 0;
};

// stringWriter_t ...
struct stringWriter_t {
    virtual R<int, error> WriteString(const string& s) = 0;
};

// byteReaderFn_t
struct byteReaderFn_t : public byteReader_t {
    byteReaderFn_t(const ReadByteFn& r) : r_(r) {}
    virtual R<byte, error> ReadByte() override { return r_(); }

   protected:
    ReadByteFn r_;
};

// byteScannerFn_t
struct byteScannerFn_t : public byteScanner_t {
    byteScannerFn_t(const ReadByteFn& r, const UnreadByteFn& f) : r_(r), f_(f) {}
    virtual R<byte, error> ReadByte() override { return r_(); }
    virtual error UnreadByte() override { return f_(); }

   protected:
    ReadByteFn r_;
    UnreadByteFn f_;
};

// byteWriterFn_t
struct byteWriterFn_t : public byteWriter_t {
    byteWriterFn_t(const WriteByteFn& w) : w_(w) {}
    virtual error WriteByte(byte c) override { return w_(c); }

   protected:
    WriteByteFn w_;
};

// runeReaderFn_t
struct runeReaderFn_t : public runeReader_t {
    runeReaderFn_t(const ReadByteFn& r) : r_(r) {}
    virtual R<rune, error> ReadRune() override { return r_(); }

   protected:
    ReadRuneFn r_;
};

// runeScannerFn_t
struct runeScannerFn_t : public runeScanner_t {
    runeScannerFn_t(const ReadRuneFn& r, const UnreadRuneFn& f) : r_(r), f_(f) {}
    virtual R<rune, error> ReadRune() override { return r_(); }
    virtual error UnreadRune() override { return f_(); }

   protected:
    ReadRuneFn r_;
    UnreadRuneFn f_;
};

// stringWriterFn_t
struct stringWriterFn_t : public stringWriter_t {
    stringWriterFn_t(const WriteStringFn& w) : w_(w) {}
    virtual R<int, error> WriteString(const string& s) override { return w_(s); }

   protected:
    WriteStringFn w_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// readerObj_t
template <typename T, typename std::enable_if<xx::has_read<T>::value, int>::type = 0>
struct readerObj_t : public reader_t {
    readerObj_t(T t) : t_(t) {}
    virtual R<int, error> Read(slice<byte> b) override {
        if (t_) {
            return t_->Read(b);
        }
        return {0, ErrEOF};
    }

   protected:
    T t_;
};

// writerObj_t
template <typename T, typename std::enable_if<xx::has_write<T>::value, int>::type = 0>
struct writerObj_t : public writer_t {
    writerObj_t(T t) : t_(t) {}
    virtual R<int, error> Write(const slice<byte> b) override {
        if (t_) {
            return t_->Write(b);
        }
        return {0, ErrEOF};
    }

   protected:
    T t_;
};

// closerObj_t
template <typename T, typename std::enable_if<xx::has_close<T>::value, int>::type = 0>
struct closerObj_t : public closer_t {
    closerObj_t(T t) : t_(t) {}
    virtual error Close() override { return t_ ? t_->Close() : nil; }

   protected:
    T t_;
};

// readWriterObj_t
template <typename T, typename std::enable_if<xx::has_read<T>::value && xx::has_write<T>::value, int>::type = 0>
struct readWriterObj_t : public readWriter_t {
    readWriterObj_t(T t) : t_(t) {}
    virtual R<int, error> Read(slice<byte> b) override {
        if (t_) {
            return t_->Read(b);
        }
        return {0, ErrEOF};
    }
    virtual R<int, error> Write(const slice<byte> b) override {
        if (t_) {
            return t_->Write(b);
        }
        return {0, ErrEOF};
    }

   protected:
    T t_;
};

// readCloserObj_t
template <typename T, typename std::enable_if<xx::has_read<T>::value && xx::has_close<T>::value, int>::type = 0>
struct readCloserObj_t : public readCloser_t {
    readCloserObj_t(T t) : t_(t) {}
    virtual R<int, error> Read(slice<byte> b) override {
        if (t_) {
            return t_->Read(b);
        }
        return {0, ErrEOF};
    }
    virtual error Close() override { return t_ ? t_->Close() : nil; }

   protected:
    T t_;
};

// writeCloserObj_t
template <typename T, typename std::enable_if<xx::has_write<T>::value && xx::has_read<T>::value, int>::type = 0>
struct writeCloserObj_t : public writeCloser_t {
    writeCloserObj_t(T t) : t_(t) {}
    virtual R<int, error> Write(const slice<byte> b) override {
        if (t_) {
            return t_->Write(b);
        }
        return {0, ErrEOF};
    }
    virtual error Close() override { return t_ ? t_->Close() : nil; }

   protected:
    T t_;
};

// readWriteCloserObj_t
template <typename T, typename std::enable_if<
                          xx::has_write<T>::value && xx::has_read<T>::value && xx::has_close<T>::value, int>::type = 0>
struct readWriteCloserObj_t : public readWriteCloser_t {
    readWriteCloserObj_t(T t) : t_(t) {}
    virtual R<int, error> Read(slice<byte> b) override {
        if (t_) {
            return t_->Read(b);
        }
        return {0, ErrEOF};
    }
    virtual R<int, error> Write(const slice<byte> b) override {
        if (t_) {
            return t_->Write(b);
        }
        return {0, ErrEOF};
    }
    virtual error Close() override { return t_ ? t_->Close() : nil; }

   protected:
    T t_;
};

// nopCloser_t ...
struct nopCloser_t : public readCloser_t {
    virtual R<int, error> Read(slice<byte> b) override { return {0, nil}; }
    virtual error Close() override { return nil; }
};

}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
// ...
using Reader = Ref<xx::reader_t>;
using Writer = Ref<xx::writer_t>;
using Closer = Ref<xx::closer_t>;
using ReadWriter = Ref<xx::readWriter_t>;
using ReadCloser = Ref<xx::readCloser_t>;
using WriteCloser = Ref<xx::writeCloser_t>;
using ReadWriteCloser = Ref<xx::readWriteCloser_t>;
//
using ByteReader = Ref<xx::byteReader_t>;
using ByteScanner = Ref<xx::byteScanner_t>;
using ByteWriter = Ref<xx::byteWriter_t>;
using RuneReader = Ref<xx::runeReader_t>;
using RuneScanner = Ref<xx::runeScanner_t>;
using StringWriter = Ref<xx::stringWriter_t>;

////////////////////////////////////////////////////////////////////////////////
//
// NewReaderFn ...
inline Reader NewReaderFn(const ReadFn& fn) { return NewRef<xx::readerFn_t>(fn); }

// NewWriterFn ...
inline Writer NewWriterFn(const WriteFn& fn) { return NewRef<xx::writerFn_t>(fn); }

// NewCloserFn ...
inline Closer NewCloserFn(const CloseFn& fn) { return NewRef<xx::closerFn_t>(fn); }

// NewReadWriterFn ...
inline ReadWriter NewReadWriterFn(const ReadFn& r, const WriteFn& w) { return NewRef<xx::readWriterFn_t>(r, w); }

// NewReadCloserFn ...
inline ReadCloser NewReadCloserFn(const ReadFn& r, const CloseFn& c) { return NewRef<xx::readCloserFn_t>(r, c); }

// NewWriteCloserFn ...
inline WriteCloser NewWriteCloserFn(const WriteFn& w, const CloseFn& c) { return NewRef<xx::writeCloserFn_t>(w, c); }

// NewReadWriteCloserFn ...
inline ReadWriteCloser NewReadWriteCloserFn(const ReadFn& r, const WriteFn& w, const CloseFn& c) {
    return NewRef<xx::readWriteCloserFn_t>(r, w, c);
}

////////////////////////////////////////////////////////////////////////////////
// NewByteReaderFn ...
inline ByteReader NewByteReaderFn(const ReadByteFn& r) { return NewRef<xx::byteReaderFn_t>(r); }

// NewByteScannerFn ...
inline ByteScanner NewByteScannerFn(const ReadByteFn& r, const UnreadByteFn& f) {
    return NewRef<xx::byteScannerFn_t>(r, f);
}

// NewByteWriterFn ...
inline ByteWriter NewByteWriterFn(const WriteByteFn& w) { return NewRef<xx::byteWriterFn_t>(w); }

// NewRuneReaderFn ...
inline RuneReader NewRuneReaderFn(const ReadRuneFn& r) { return NewRef<xx::runeReaderFn_t>(r); }

// NewRuneScannerFn ...
inline RuneScanner NewByteScannerFn(const ReadRuneFn& r, const UnreadRuneFn& f) {
    return NewRef<xx::runeScannerFn_t>(r, f);
}

// NewStringWriterFn ...
inline StringWriter NewStringWriterFn(const WriteStringFn& w) { return NewRef<xx::stringWriterFn_t>(w); }

////////////////////////////////////////////////////////////////////////////////
//
// NewReader ...
template <typename T, typename std::enable_if<xx::has_read<T>::value, int>::type = 0>
Reader NewReader(T t) {
    return NewRef<xx::readerObj_t<T>>(t);
}

// NewWriter ...
template <typename T, typename std::enable_if<xx::has_write<T>::value, int>::type = 0>
Writer NewWriter(T t) {
    return NewRef<xx::writerObj_t<T>>(t);
}

// NewCloser ...
template <typename T, typename std::enable_if<xx::has_close<T>::value, int>::type = 0>
Closer NewCloser(T t) {
    return NewRef<xx::closerObj_t<T>>(t);
}

// NewReadWriter ...
template <typename T, typename std::enable_if<xx::has_read<T>::value && xx::has_write<T>::value, int>::type = 0>
ReadWriter NewReadWriter(T t) {
    return NewRef<xx::readWriterObj_t<T>>(t);
}

// NewReadCloser ...
template <typename T, typename std::enable_if<xx::has_read<T>::value && xx::has_close<T>::value, int>::type = 0>
ReadCloser NewReadCloser(T t) {
    return NewRef<xx::readCloserObj_t<T>>(t);
}

// NewWriteCloser ...
template <typename T, typename std::enable_if<xx::has_write<T>::value && xx::has_read<T>::value, int>::type = 0>
WriteCloser NewWriteCloser(T t) {
    return NewRef<xx::writeCloserObj_t<T>>(t);
}

// NewReadWriteCloser ...
template <typename T, typename std::enable_if<
                          xx::has_write<T>::value && xx::has_read<T>::value && xx::has_close<T>::value, int>::type = 0>
ReadWriteCloser NewReadWriteCloser(T t) {
    return NewRef<xx::readWriteCloserObj_t<T>>(t);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A LimitedReader reads from R but limits the amount of
// data returned to just N bytes. Each call to Read
// updates N to reflect the new amount remaining.
// Read returns EOF when N <= 0 or when the underlying R returns EOF.
struct LimitedReader : public xx::reader_t {
    Reader R;    // underlying reader
    int64 N{0};  // max bytes remaining

    LimitedReader(Reader r, int64 n) : R(r), N(n) {}

    // Read ...
    virtual gx::R<int, error> Read(slice<byte> p) override {
        if (N <= 0) {
            return {0, ErrEOF};
        }
        if (int64(len(p)) > N) {
            p = p(0, N);
        }
        AUTO_R(n, err, R->Read(p));
        N -= int64(n);
        return {n, err};
    }
};

}  // namespace io
}  // namespace gx
