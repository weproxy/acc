//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/time/time.h"
#include "gx/x/time/rate/rate.h"
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
R<int, error> ReadFull(Reader r, byte_s buf) {
    int nr = 0;
    while (nr < buf.size()) {
        AUTO_R(n, err, r->Read(buf(nr, buf.size() - nr)));
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
    R<int, error> Write(const byte_s b) { return {b.size(), nil}; }
};
}  // namespace xx

// Discard ...
extern std::shared_ptr<xx::discard_t> Discard;

}  // namespace io
}  // namespace gx

#include "copy.h"
