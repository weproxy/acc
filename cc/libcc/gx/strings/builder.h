//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"

namespace gx {
namespace strings {
////////////////////////////////////////////////////////////////////////////////
// Builder ...
struct Builder {
   protected:
    slice<byte> buf;

    void grow(int n) {
        slice<byte> b = make(len(buf), 2 * cap(buf) + n);
        copy(b, buf);
        buf = b;
    }

   public:
    int Len() const { return len(buf); }
    int Cap() const { return cap(buf); }

    void Reset() {
        buf._reset();
    }

    void Grow(int n) {
        if (cap(buf) - len(buf) < n) {
            grow(n);
        }
    }

    R<int, error> Write(slice<byte> p) {
        buf = append(buf, p);
        return {len(p), nil};
    }

    error WriteByte(byte c) {
        buf = append(buf, c);
        return nil;
    }

    R<int, error> WriteString(const string& s) {
        buf = append(buf, s);
        return {len(s), nil};
    }

    string String() const { return string(buf); }
};

}  // namespace strings
}  // namespace gx
