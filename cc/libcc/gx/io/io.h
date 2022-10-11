//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "time/time.h"
#include "x/time/rate/rate.h"
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
// ICloser  ...
struct ICloser {
    ICloser() = default;
    virtual ~ICloser() { Close(); }
    virtual void Close(){};
};
typedef std::shared_ptr<ICloser> Closer;

namespace xx {
typedef std::function<void()> CloserFn;
struct WrapCloser : public ICloser {
    WrapCloser(const CloserFn& fn) : fn_(fn) {}
    void Close() {
        if (fn_) fn_();
    }
    CloserFn fn_;
};
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
// NewCloserFn ..
inline Closer NewCloserFn(const xx::CloserFn&& fn) {
    auto p = std::shared_ptr<xx::WrapCloser>(new xx::WrapCloser(fn));
    return p;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadFull ..
template <typename Reader, typename std::enable_if<xx::is_reader<Reader>::value, int>::type = 0>
R<size_t, error> ReadFull(Reader r, void* buf, size_t len) {
    size_t nr = 0;
    while (nr < len) {
        AUTO_R(n, err, r->Read((uint8*)buf + nr, len - nr));
        if (n > 0) {
            nr += n;
        }
        if (err) {
            return {nr, err};
        }
    }
    return {nr, nil};
}

namespace xx {
struct discard_t {
    R<size_t, error> Write(const void*, size_t n) { return {n, nil}; }
};
}  // namespace xx

// Discard ...
extern xx::discard_t Discard;

}  // namespace io
}  // namespace gx

#include "copy.h"
