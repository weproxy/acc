//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "netio.h"

#include "gx/time/time.h"

namespace nx {
namespace netio {

// Copy ...
R<size_t /*w*/, error> Copy(net::Conn w, net::Conn r, CopyOption opt) {
    byte_s buf = make(1024 * 32);

    size_t written = 0;
    error rerr;

    for (;;) {
        AUTO_R(nr, err, r->Read(buf));
        if (nr > 0) {
            if (opt.Limit) {
                auto rv = opt.Limit->ReserveN(time::Now(), (int)nr);
                if (rv.OK()) {
                    auto tm = rv.Delay();
                    if (tm > 0) {
                        time::Sleep(time::Millisecond * tm);
                    }
                }
            }

            AUTO_R(nw, er2, w->Write(buf(0, nr)));
            if (nw > 0) {
                written += nw;
                if (opt.CopingFn) {
                    opt.CopingFn(nw);
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

    println("io::Copy(", r, "->", w, ")", written, "bytes");

    return {written, rerr};
}

// Relay ...
R<size_t /*r*/, size_t /*w*/, error> Relay(net::Conn c, net::Conn rc, RelayOption opt) {
    WaitGroup wg(1);

    size_t written = 0;
    error werr;

    // rc <- c
    gx::go([&] {
        DEFER([&] {
            rc->CloseWrite();
            rc->SetReadDeadline(time::Now());
            wg.Done();
        }());

        if (opt.ToWrite.Data) {
            AUTO_R(w, e, rc->Write(opt.ToWrite.Data));
            if (e) {
                written += w;
                werr = e;
                return;
            }
        }

        AUTO_R(w, e, netio::Copy(rc, c, opt.Read));
        written += w;
        werr = e;
    });

    // c <- rc
    AUTO_R(read, rerr, netio::Copy(c, rc, opt.Write));

    c->CloseWrite();
    c->SetReadDeadline(time::Now());

    wg.Wait();

    return {read, written, werr ? werr : rerr};
}

// Copy ...
R<size_t /*w*/, error> Copy(net::PacketConn w, net::PacketConn r, CopyOption opt) {
    byte_s buf = make(1024 * 4);

    size_t written = 0;
    error rerr;

    for (;;) {
        AUTO_R(nr, addr, err, r->ReadFrom(buf));
        if (nr > 0) {
            AUTO_R(nw, er2, w->WriteTo(buf(0, nr), addr));
            if (nw > 0) {
                written += nw;
                if (opt.CopingFn) {
                    opt.CopingFn(nw);
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

    println("io::Copy(", r, "->", w, ")", written, "bytes");

    return {written, rerr};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Relay ...
R<size_t /*r*/, size_t /*w*/, error> Relay(net::PacketConn c, net::PacketConn rc, RelayOption opt) {
    WaitGroup wg(1);

    size_t written = 0;
    error werr;

    // rc <- c
    gx::go([&] {
        DEFER([&] {
            rc->CloseWrite();
            rc->SetReadDeadline(time::Now());
            wg.Done();
        }());

        if (opt.ToWrite.Data && opt.ToWrite.Addr) {
            AUTO_R(w, e, rc->WriteTo(opt.ToWrite.Data, opt.ToWrite.Addr));
            if (e) {
                written += w;
                werr = e;
                return;
            }
        }

        AUTO_R(w, e, netio::Copy(rc, c, opt.Read));
        written += w;
        werr = e;
    });

    // c <- rc
    AUTO_R(read, rerr, netio::Copy(c, rc, opt.Write));

    c->CloseWrite();
    c->SetReadDeadline(time::Now());

    wg.Wait();

    return {read, written, werr ? werr : rerr};
}

}  // namespace netio
}  // namespace nx
