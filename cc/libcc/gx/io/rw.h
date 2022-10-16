//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "io.h"
#include "xx.h"

namespace gx {
namespace io {
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
    virtual void Close() = 0;
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
    virtual R<int, error> Read(slice<byte> b) override {
        if (r_) {
            return r_(b);
        }
        return {0, ErrEOF};
    }

   protected:
    ReadFn r_;
};

// writerFn_t
struct writerFn_t : public writer_t {
    writerFn_t(const WriteFn& w) : w_(w) {}
    virtual R<int, error> Write(const slice<byte> b) override {
        if (w_) {
            return w_(b);
        }
        return {0, ErrEOF};
    }

   protected:
    WriteFn w_;
};

// closerFn_t
struct closerFn_t : public closer_t {
    closerFn_t(const CloseFn& c) : c_(c) {}
    virtual void Close() override {
        if (c_) {
            c_();
        }
    }

   protected:
    CloseFn c_;
};

// readWriterFn_t
struct readWriterFn_t : public readWriter_t {
    readWriterFn_t(const ReadFn& r, const WriteFn& w) : r_(r), w_(w) {}
    virtual R<int, error> Read(slice<byte> b) override {
        if (r_) {
            return r_(b);
        }
        return {0, ErrEOF};
    }
    virtual R<int, error> Write(const slice<byte> b) override {
        if (w_) {
            return w_(b);
        }
        return {0, ErrEOF};
    }

   protected:
    ReadFn r_;
    WriteFn w_;
};

// readCloserFn_t
struct readCloserFn_t : public readCloser_t {
    readCloserFn_t(const ReadFn& r, const CloseFn& c) : r_(r), c_(c) {}
    virtual R<int, error> Read(slice<byte> b) override {
        if (r_) {
            return r_(b);
        }
        return {0, ErrEOF};
    }
    virtual void Close() override {
        if (c_) {
            c_();
        }
    }

   protected:
    ReadFn r_;
    CloseFn c_;
};

// writeCloserFn_t
struct writeCloserFn_t : public writeCloser_t {
    writeCloserFn_t(const WriteFn& w, const CloseFn& c) : w_(w_), c_(c) {}
    virtual R<int, error> Write(const slice<byte> b) override {
        if (w_) {
            return w_(b);
        }
        return {0, ErrEOF};
    }
    virtual void Close() override {
        if (c_) {
            c_();
        }
    }

   protected:
    WriteFn w_;
    CloseFn c_;
};

// readWriteCloserFn_t
struct readWriteCloserFn_t : public readWriteCloser_t {
    readWriteCloserFn_t(const ReadFn& r, const WriteFn& w, const CloseFn& c) : r_(r), w_(w), c_(c) {}
    virtual R<int, error> Read(slice<byte> b) override {
        if (r_) {
            return r_(b);
        }
        return {0, ErrEOF};
    }
    virtual R<int, error> Write(const slice<byte> b) override {
        if (w_) {
            return w_(b);
        }
        return {0, ErrEOF};
    }
    virtual void Close() override {
        if (c_) {
            c_();
        }
    }

   protected:
    ReadFn r_;
    WriteFn w_;
    CloseFn c_;
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
    virtual void Close() override {
        if (t_) {
            t_->Close();
        }
    }

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
    virtual void Close() override {
        if (t_) {
            t_->Close();
        }
    }

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
    virtual void Close() override {
        if (t_) {
            t_->Close();
        }
    }

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
    virtual void Close() override {
        if (t_) {
            t_->Close();
        }
    }

   protected:
    T t_;
};

}  // namespace xx
}  // namespace io
}  // namespace gx
