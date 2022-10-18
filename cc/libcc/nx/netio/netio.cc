//
// weproxy@foxmail.com 2022/10/03
//

#include "netio.h"

#include "gx/time/time.h"

namespace nx {
namespace netio {

// Copy ...
R<size_t /*w*/, error> Copy(net::Conn w, net::Conn r, CopyOption opt) {
    slice<byte> buf = make(1024 * 32);

    size_t written = 0;
    error rerr;

    for (;;) {
        if (opt.ReadTimeout) {
            r->SetReadDeadline(time::Now().Add(opt.ReadTimeout));
        }
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

            if (opt.WriteTimeout) {
                w->SetWriteDeadline(time::Now().Add(opt.WriteTimeout));
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

    // println("io::Copy(", r, "->", w, ")", written, "bytes");

    return {written, rerr};
}

// Relay ...
error Relay(net::Conn a, net::Conn b, RelayOption opt) {
    WaitGroup wg(1);

    error errA2B;

    // a -> b
    gx::go([&] {
        DEFER({
            b->CloseWrite();
            b->SetReadDeadline(time::Now());
            wg.Done();
        });

        if (opt.ToB.Data) {
            if (opt.A2B.WriteTimeout) {
                b->SetWriteDeadline(time::Now().Add(opt.A2B.WriteTimeout));
            }
            AUTO_R(_, e, b->Write(opt.ToB.Data));
            if (e) {
                errA2B = e;
                return;
            }
        }

        AUTO_R(_, e, netio::Copy(b, a, opt.A2B));
        errA2B = e;
    });

    // a <- b
    AUTO_R(_, errB2A, netio::Copy(a, b, opt.B2A));

    a->CloseWrite();
    a->SetReadDeadline(time::Now());

    wg.Wait();

    return errA2B ? errA2B : errB2A;
}

// Copy ...
R<size_t /*w*/, error> Copy(net::PacketConn w, net::PacketConn r, CopyOption opt) {
    slice<byte> buf = make(1024 * 4);

    size_t written = 0;
    error rerr;

    for (;;) {
        if (opt.ReadTimeout) {
            r->SetReadDeadline(time::Now().Add(opt.ReadTimeout));
        }
        AUTO_R(nr, addr, err, r->ReadFrom(buf));
        if (nr > 0) {
            if (opt.WriteTimeout) {
                w->SetWriteDeadline(time::Now().Add(opt.WriteTimeout));
            }
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

    // println("io::Copy(", r, "->", w, ")", written, "bytes");

    return {written, rerr};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Relay ...
error Relay(net::PacketConn a, net::PacketConn b, RelayOption opt) {
    WaitGroup wg(1);

    error errA2B;

    // a -> b
    gx::go([&] {
        DEFER({
            b->CloseWrite();
            b->SetReadDeadline(time::Now());
            wg.Done();
        });

        if (opt.ToB.Data && opt.ToB.Addr) {
            if (opt.A2B.WriteTimeout) {
                b->SetWriteDeadline(time::Now().Add(opt.A2B.WriteTimeout));
            }
            AUTO_R(_, e, b->WriteTo(opt.ToB.Data, opt.ToB.Addr));
            if (e) {
                errA2B = e;
                return;
            }
        }

        AUTO_R(_, e, netio::Copy(b, a, opt.A2B));
        errA2B = e;
    });

    // a <- b
    AUTO_R(_, errB2A, netio::Copy(a, b, opt.B2A));

    a->CloseWrite();
    a->SetReadDeadline(time::Now());

    wg.Wait();

    return errA2B ? errA2B : errB2A;
}

}  // namespace netio
}  // namespace nx
