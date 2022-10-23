//
// weproxy@foxmail.com 2022/10/20
//

package dns

import (
	"encoding/binary"
	"errors"
	"fmt"
	"net"
	"strings"
	"time"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/dns"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/stats"
)

////////////////////////////////////////////////////////////////////////////////

// key_t ...
type key_t uint64

// makeKey ...
func makeKeyIPPort(ip net.IP, port uint16) key_t {
	a := binary.LittleEndian.Uint32(ip)
	b := uint32(port)
	return key_t(a)<<16 | key_t(b)
}

func makeKey(addr net.Addr) key_t {
	switch addr := addr.(type) {
	case *net.TCPAddr:
		return makeKeyIPPort(addr.IP, uint16(addr.Port))
	case *net.UDPAddr:
		return makeKeyIPPort(addr.IP, uint16(addr.Port))
	}
	return 0
}

// //////////////////////////////////////////////////////////////////////////////
// udpSess_t ...
//
// <NAT-Open Notice> maybe one source client send to multi-target server, likes:
//
//	caddr=1.2.3.4:100 --> raddr=8.8.8.8:53
//	                  --> raddr=4.4.4.4:53
type udpSess_t struct {
	ln            net.PacketConn // to source client, it is net.PacketConn
	rc            net.PacketConn // to target server, it is udpConn_t
	caddr         net.Addr       // source client addr
	sta           *stats.Stats
	afterClosedFn func()
}

// Close ...
func (m *udpSess_t) Close() error {
	if m.rc != nil {
		m.rc.Close()
		m.sta.Done("closed")
		m.rc = nil
		if m.afterClosedFn != nil {
			m.afterClosedFn()
		}
	}
	return nil
}

// Start ...
func (m *udpSess_t) Start(target string) {
	tag := fmt.Sprintf("%s DNS_%v %v->%s", TAG, nx.NewID(), m.caddr, target)
	sta := stats.NewUDPStats(stats.TypeS5, tag)

	sta.Start("started")
	m.sta = sta

	// loopRecvRC
	go func() {
		defer m.Close()

		var opt netio.CopyOption
		opt.WriteAddr = m.caddr
		opt.MaxTimes = 1 // read 1 packet then close
		opt.ReadTimeout = time.Second * 5
		opt.CopingFn = func(n int) { m.sta.AddSent(int64(n)) }

		// copy to ln from rc
		netio.CopyPacket(m.ln, m.rc, opt)
	}()
}

// WriteToRC ...
func (m *udpSess_t) WriteToRC(buf []byte, raddr net.Addr) error {
	if m.rc == nil {
		return net.ErrClosed
	}

	// buf is from source client
	m.sta.AddRecv(int64(len(buf)))

	// udpConn_t.WriteTo()
	// unpack data (from source client), and writeTo target server
	_, err := m.rc.WriteTo(buf, raddr)

	return err
}

// udpSessMap ...
type udpSessMap map[key_t]*udpSess_t

////////////////////////////////////////////////////////////////////////////////

// udpConn_t
// to target server
type udpConn_t struct {
	net.PacketConn
	// typ socks.AddrType
}

// ReadFrom override
// readFrom target server, and pack data (to client)
func (m *udpConn_t) ReadFrom(buf []byte) (n int, addr net.Addr, err error) {
	n, addr, err = m.PacketConn.ReadFrom(buf)
	if err != nil {
		return
	}

	// cache store
	dns.OnResponse(buf[:n])

	return
}

// WriteTo override
// unpack data (from source client), and writeTo target server
func (m *udpConn_t) WriteTo(buf []byte, addr net.Addr) (n int, err error) {
	n, err = m.PacketConn.WriteTo(buf, addr)

	return
}

// //////////////////////////////////////////////////////////////////////////////

// runServLoop ...
func runServLoop(ln net.PacketConn) error {
	// sessMap
	sessMap := make(udpSessMap)
	defer func() {
		for _, v := range sessMap {
			v.Close()
		}
	}()

	defer ln.Close()

	buf := make([]byte, 1024*8)

	for {
		// read
		n, caddr, err := ln.ReadFrom(buf)
		if err != nil {
			if !errors.Is(err, net.ErrClosed) {
				logx.E("%s err: %v", TAG, err)
			}
			break
		} else if n <= 0 {
			continue
		}

		// packet data
		data := buf[:n]

		// cache query
		msg, ans, err := dns.OnRequest(data)
		if err == nil && ans != nil && len(ans.Data) > 0 {
			// cache answer
			ln.WriteTo(ans.Data, caddr)
			continue
		}

		// lookfor or create sess
		var sess *udpSess_t

		// use source client addr as key
		// <TODO-Notice>
		//  some user's env has multi-outbound-ip, we will get diff caddr although he use one-same-conn
		key := makeKey(caddr)

		if v, ok := sessMap[key]; !ok {
			// create a new rc for every new source client
			c, err := net.ListenPacket("udp", ":0")
			if err != nil {
				logx.E("%s err: %v", TAG, err)
				continue
			}

			target := func() string {
				if msg != nil && len(msg.Questions) > 0 {
					return strings.TrimSuffix(msg.Questions[0].Name.String(), ".")
				}
				return ""
			}()

			// wrap as udpConn_t
			rc := &udpConn_t{PacketConn: c}

			// store to sessMap
			sess = &udpSess_t{
				ln:    ln,
				rc:    rc,
				caddr: caddr,
			}
			sessMap[key] = sess

			// remove it after rc closed
			sess.afterClosedFn = func() { delete(sessMap, key) }

			// start recv loop...
			sess.Start(target)
		} else {
			sess = v
		}

		// DNS server
		raddr := &net.UDPAddr{IP: net.IPv4(223, 5, 5, 5), Port: 53}

		// writeTo target server
		err = sess.WriteToRC(data, raddr)
		if err != nil {
			return err
		}
	}

	return nil
}
