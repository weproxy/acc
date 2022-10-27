//
// weproxy@foxmail.com 2022/10/20
//

package netio

import (
	"io"
	"net"
	"sync"
	"time"

	"golang.org/x/time/rate"
)

// Packet ...
type Packet struct {
	Addr net.Addr
	Data []byte
}

// CopingFn ...
type CopingFn func(int)

// CopyOption ...
type CopyOption struct {
	Limit        *rate.Limiter
	CopingFn     CopingFn
	ReadTimeout  time.Duration
	WriteTimeout time.Duration
	WriteAddr    net.Addr
	MaxTimes     int // max copy times
}

// RelayOption ...
type RelayOption struct {
	A2B CopyOption
	B2A CopyOption
	ToB Packet
}

// Copy ...
func Copy(w net.Conn, r net.Conn, opt CopyOption) (written int64, copyErr error) {
	buf := make([]byte, 1024*32)

	for {
		if opt.ReadTimeout > 0 {
			r.SetReadDeadline(time.Now().Add(opt.ReadTimeout))
		}
		nr, err := r.Read(buf)
		if nr > 0 {
			if opt.Limit != nil {
				if rv := opt.Limit.ReserveN(time.Now(), int(nr)); rv.OK() {
					if tm := rv.Delay(); tm > 0 {
						time.Sleep(time.Millisecond * tm)
					}
				}
			}

			if opt.WriteTimeout > 0 {
				w.SetWriteDeadline(time.Now().Add(opt.WriteTimeout))
			}
			nw, er2 := w.Write(buf[0:nr])
			if nw > 0 {
				written += int64(nw)
				if opt.CopingFn != nil {
					opt.CopingFn(nw)
				}
			}
			if er2 != nil && err == nil {
				err = er2
			}
		}

		if err != nil {
			if err != io.EOF {
				copyErr = err
			}
			break
		}
	}

	// println("io::Copy(", r, "->", w, ")", written, "bytes");

	return
}

// Relay ...
func Relay(a net.Conn, b net.Conn, opt RelayOption) (err error) {
	var wg sync.WaitGroup

	var errA2B error

	// a -> b
	wg.Add(1)
	go func() {
		defer func() {
			if t, ok := b.(*net.TCPConn); ok {
				t.CloseWrite()
			}
			b.SetReadDeadline(time.Now())
			wg.Done()
		}()

		if len(opt.ToB.Data) > 0 {
			if opt.A2B.WriteTimeout > 0 {
				b.SetWriteDeadline(time.Now().Add(opt.A2B.WriteTimeout))
			}
			nw, err := b.Write(opt.ToB.Data)
			if err != nil {
				errA2B = err
				return
			}
			if nw > 0 && opt.A2B.CopingFn != nil {
				opt.A2B.CopingFn(nw)
			}
		}

		_, errA2B = Copy(b, a, opt.A2B)
	}()

	// a <- b
	_, errB2A := Copy(a, b, opt.B2A)

	if t, ok := a.(*net.TCPConn); ok {
		t.CloseWrite()
	}
	a.SetReadDeadline(time.Now())

	wg.Wait()

	if errA2B != nil {
		return errA2B
	}
	return errB2A
}

// CopyPacket ...
func CopyPacket(w net.PacketConn, r net.PacketConn, opt CopyOption) (written int64, copyErr error) {
	buf := make([]byte, 1024*8)

	times := 0
	for {
		if opt.ReadTimeout > 0 {
			r.SetReadDeadline(time.Now().Add(opt.ReadTimeout))
		}
		nr, addr, err := r.ReadFrom(buf)
		if nr > 0 {
			if opt.WriteTimeout > 0 {
				w.SetWriteDeadline(time.Now().Add(opt.WriteTimeout))
			}

			raddr := opt.WriteAddr
			if raddr == nil {
				raddr = addr
			}

			if opt.MaxTimes <= 0 {
				if uaddr, ok := raddr.(*net.UDPAddr); ok && (uaddr.Port == 53 || uaddr.Port == 23) {
					opt.MaxTimes = 1
				}
			}

			nw, er2 := w.WriteTo(buf[0:nr], raddr)
			if nw > 0 {
				written += int64(nw)
				if opt.CopingFn != nil {
					opt.CopingFn(nw)
				}
			}
			if er2 != nil && err == nil {
				err = er2
			}

			times++
		}

		if err != nil {
			if err != io.EOF {
				copyErr = err
			}
			break
		}

		if opt.MaxTimes > 0 && times >= opt.MaxTimes {
			// read MaxTimes packets then close
			break
		}
	}

	// println("io::Copy(", r, "->", w, ")", written, "bytes");

	return
}

// RelayPacket ...
func RelayPacket(a net.PacketConn, b net.PacketConn, opt RelayOption) (err error) {
	var wg sync.WaitGroup

	var errA2B error

	// a -> b
	wg.Add(1)
	go func() {
		defer func() {
			// b.Close()
			b.SetReadDeadline(time.Now())
			wg.Done()
		}()

		if len(opt.ToB.Data) > 0 {
			if opt.A2B.WriteTimeout > 0 {
				b.SetWriteDeadline(time.Now().Add(opt.A2B.WriteTimeout))
			}
			nw, err := b.WriteTo(opt.ToB.Data, opt.ToB.Addr)
			if err != nil {
				errA2B = err
				return
			}
			if nw > 0 && opt.A2B.CopingFn != nil {
				opt.A2B.CopingFn(nw)
			}
		}

		_, errA2B = CopyPacket(b, a, opt.A2B)
	}()

	// a <- b
	_, errB2A := CopyPacket(a, b, opt.B2A)

	// a.Close()
	a.SetReadDeadline(time.Now())

	wg.Wait()

	if errA2B != nil {
		return errA2B
	}
	return errB2A
}
