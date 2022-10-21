//
// weproxy@foxmail.com 2022/10/20
//

package s5

import (
	"encoding/binary"
	"fmt"
	"net"
	"time"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/socks"
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
		if m.afterClosedFn != nil {
			m.afterClosedFn()
		}
		m.sta.Done("closed")
		m.rc = nil
	}
	return nil
}

// Start ...
func (m *udpSess_t) Start() {
	tag := fmt.Sprintf("%s UDP_%v %v", TAG, nx.NewID(), m.caddr)
	sta := stats.NewUDPStats(stats.TypeS5, tag)

	sta.Start("started")
	m.sta = sta

	// loopRecvRC
	go func() {
		defer m.Close()

		var opt netio.CopyOption
		opt.WriteAddr = m.caddr
		opt.CopingFn = func(n int) { m.sta.AddSent(int64(n)) }

		// copy to ln from rc
		netio.CopyPacket(m.ln, m.rc, opt)
	}()
}

// WriteToRC ...
func (m *udpSess_t) WriteToRC(buf []byte) error {
	if m.rc == nil {
		return net.ErrClosed
	}

	// buf is from source client
	m.sta.AddRecv(int64(len(buf)))

	// udpConn_t.WriteTo()
	// unpack data (from source client), and writeTo target server
	_, err := m.rc.WriteTo(buf, nil)

	return err
}

// udpSessMap ...
type udpSessMap map[key_t]*udpSess_t

////////////////////////////////////////////////////////////////////////////////

// udpConn_t
// to target server
type udpConn_t struct {
	net.PacketConn
	typ_ socks.AddrType
}

// ReadFrom ...
// readFrom target server, and pack data (to client)
//
// <<< RSP:
//
//	| RSV | FRAG | ATYP | SRC.ADDR | SRC.PORT | DATA |
//	+-----+------+------+----------+----------+------+
//	|  2  |  1   |  1   |    ...   |    2     |  ... |
func (m *udpConn_t) ReadFrom(buf []byte) (int, net.Addr, error) {
	if socks.AddrTypeIPv4 != m.typ_ && socks.AddrTypeIPv6 != m.typ_ {
		return 0, nil, socks.ErrInvalidAddrType
	}

	// readFrom target server
	pos := 3 + 1 + 2
	if socks.AddrTypeIPv4 == m.typ_ {
		pos += net.IPv4len
	} else {
		pos += net.IPv6len
	}
	n, addr, err := m.PacketConn.ReadFrom(buf[pos:])
	if err != nil {
		return n, addr, err
	}

	// pack data

	raddr := socks.FromNetAddr(addr)
	copy(buf[3:], raddr.B)

	n += 3 + len(raddr.B)

	buf[0] = 0 // RSV
	buf[1] = 0 //
	buf[2] = 0 // FRAG

	return n, addr, nil
}

// WriteTo ...
// unpack data (from source client), and writeTo target server
//
// >>> REQ:
//
//	| RSV | FRAG | ATYP | DST.ADDR | DST.PORT | DATA |
//	+-----+------+------+----------+----------+------+
//	|  2  |  1   |  1   |    ...   |    2     |  ... |
func (m *udpConn_t) WriteTo(buf []byte, addr net.Addr) (int, error) {
	if len(buf) < 10 {
		return 0, socks.ErrInvalidSocksVersion
	}

	// unpack data

	raddr, err := socks.ParseAddr(buf[3:])
	if err != nil {
		return 0, err
	}

	m.typ_ = socks.AddrType(raddr.B[0])
	if socks.AddrTypeIPv4 != m.typ_ && socks.AddrTypeIPv6 != m.typ_ {
		return 0, socks.ErrInvalidAddrType
	}

	// writeTo target server
	pos := 3 + len(raddr.B)
	n, err := m.PacketConn.WriteTo(buf[pos:], raddr.ToUDPAddr())
	if err != nil {
		return n, err
	}

	return n + pos, nil
}

// //////////////////////////////////////////////////////////////////////////////
// handleUDP ...
//
// <NAT-Open Notice> maybe one source client send to multi-target server, likes:
//
//	caddr=1.2.3.4:100 -. raddr=8.8.8.8:53
//	                  -. raddr=4.4.4.4:53
func handleUDP(ln net.PacketConn, caddr, raddr net.Addr) error {
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
		// 5 minutes timeout
		ln.SetReadDeadline(time.Now().Add(time.Minute * 5))

		// read
		n, caddr, err := ln.ReadFrom(buf)
		if err != nil {
			if err != net.ErrClosed {
				logx.E("%s err: %v", TAG, err)
			}
			break
		} else if n <= 0 {
			continue
		}

		// packet data
		data := buf[:n]

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
			sess.Start()
		} else {
			sess = v
		}

		// writeTo target server
		err = sess.WriteToRC(data)
		if err != nil {
			return err
		}
	}

	return nil
}