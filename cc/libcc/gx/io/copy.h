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
// OnCopyFn ...
typedef std::function<void(size_t)> OnCopyFn;

// CopyOption ...
struct CopyOption {
    rate::Limiter Limit;
    OnCopyFn OnCopy;
};

// RelayOption ...
struct RelayOption {
    CopyOption Read;
    CopyOption Write;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copy ...
template <typename ReadCloser, typename WriteCloser,
          typename std::enable_if<xx::is_read_closer<ReadCloser>::value && xx::is_write_closer<WriteCloser>::value,
                                  int>::type = 0>
R<size_t /*written*/, error> Copy(ReadCloser r, WriteCloser w, CopyOption opt = {}) {
    DEFER(r->Close());
    // DEFER(w->Close()); // ?

    byte_s buf = make(1024 * 32);

    size_t witten = 0;
    error rerr;

    for (;;) {
        AUTO_R(nr, err, r->Read(buf));
        if (nr > 0) {
            if (opt.Limit) {
                auto rv = opt.Limit->ReserveN(time::Now(), (int)nr);
                if (rv.OK()) {
                    auto tm = rv.Delay();
                    if (tm > 0) {
                        // sleep::ms(tm);
                        time::Sleep(time::Millisecond * tm);
                    }
                }
            }

            AUTO_R(nw, er2, w->Write(buf(0, nr)));
            if (nw > 0) {
                witten += nw;
                if (opt.OnCopy) {
                    opt.OnCopy(nw);
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

    // println("io::Copy(", r, "->", w, ")", witten, "bytes");

    return {witten, rerr};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Relay ...
template <typename ReadWriteCloserA, typename ReadWriteCloserB,
          typename std::enable_if<xx::is_read_write_closer<ReadWriteCloserA>::value &&
                                      xx::is_read_write_closer<ReadWriteCloserB>::value,
                                  int>::type = 0>
R<size_t /*read*/, size_t /*written*/, error> Relay(ReadWriteCloserA a, ReadWriteCloserB b, RelayOption opt = {}) {
    WaitGroup wg(1);

    size_t written = 0;
    error werr;

    // a -> b
    gx::go([&] {
        DEFER(wg.Done());

        AUTO_R(w, e, Copy(a, b, opt.Read));
        written = w;
        werr = e;
    });

    // a <- b
    AUTO_R(read, rerr, Copy(b, a, opt.Write));

    wg.Wait();

    return {read, written, werr ? werr : rerr};
}

}  // namespace io
}  // namespace gx
