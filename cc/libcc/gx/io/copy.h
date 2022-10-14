//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "xx.h"

namespace gx {
namespace io {

////////////////////////////////////////////////////////////////////////////////////////////////////
// CopingFn ...
typedef std::function<void(int /*w*/)> CopingFn;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copy ...
template <typename Writer, typename Reader,
          typename std::enable_if<xx::is_writer<Writer>::value && xx::is_reader<Reader>::value,
                                  int>::type = 0>
R<size_t /*w*/, error> Copy(Writer w, Reader r, const CopingFn& copingFn = {}) {
    byte_s buf = make(1024 * 32);

    size_t witten = 0;
    error rerr;

    for (;;) {
        AUTO_R(nr, err, r->Read(buf));
        if (nr > 0) {
            AUTO_R(nw, er2, w->Write(buf(0, nr)));
            if (nw > 0) {
                witten += nw;
                if (copingFn) {
                    copingFn(nw);
                }
            }
            if (er2 && !err) {
                err = er2;
            }
        }

        if (err) {
            rerr = err;
            break;
        }
    }

    return {witten, rerr};
}

}  // namespace io
}  // namespace gx
