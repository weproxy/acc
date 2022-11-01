//
// weproxy@foxmail.com 2022/10/20
//

package s5

import (
	"errors"
	"fmt"
	"net"
	"time"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/dns"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/socks"
	"weproxy/acc/libgo/nx/stats"
)

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
// unpackedBuf is from source client
func (m *udpSess_t) WriteToRC(unpackedBuf []byte, raddr net.Addr) error {
	if m.rc == nil {
		return net.ErrClosed
	}

	m.sta.AddRecv(int64(len(unpackedBuf)))

	// udpConn_t.WriteTo target server
	_, err := m.rc.WriteTo(unpackedBuf, raddr)

	return err
}

// udpSessMap ...
type udpSessMap map[nx.Key]*udpSess_t

////////////////////////////////////////////////////////////////////////////////

// udpConn_t
// to target server
type udpConn_t struct {
	net.PacketConn
	typ socks.AddrType
}

// ReadFrom override
// readFrom target server, and pack data (to client)
//
// <<< RSP:
//
//	| RSV | FRAG | ATYP | SRC.ADDR | SRC.PORT | DATA |
//	+-----+------+------+----------+----------+------+
//	|  2  |  1   |  1   |    ...   |    2     |  ... |
func (m *udpConn_t) ReadFrom(buf []byte) (int, net.Addr, error) {
	if socks.AddrTypeIPv4 != m.typ && socks.AddrTypeIPv6 != m.typ {
		return 0, nil, socks.ErrInvalidAddrType
	}

	// readFrom target server
	pos := 3 + 1 + 2
	if socks.AddrTypeIPv4 == m.typ {
		pos += net.IPv4len
	} else {
		pos += net.IPv6len
	}
	n, addr, err := m.PacketConn.ReadFrom(buf[pos:])
	if err != nil {
		return n, addr, err
	}

	// dns cache store
	if addr, ok := addr.(*net.UDPAddr); ok && addr.Port == 53 {
		dns.OnResponse(buf[pos : pos+n])
	}

	// pack data
	raddr := socks.FromNetAddr(addr)
	copy(buf[3:], raddr.B)
	buf[0], buf[1], buf[2] = 0, 0, 0 // RSV|FRAG

	return 3 + len(raddr.B) + n, addr, nil
}

// WriteTo override
// unpackedBuf from source client, and writeTo target server
func (m *udpConn_t) WriteTo(unpackedBuf []byte, raddr net.Addr) (int, error) {
	return m.PacketConn.WriteTo(unpackedBuf, raddr)
}

// //////////////////////////////////////////////////////////////////////////////
// handleUDP ...
//
// <NAT-Open Notice> maybe one source client send to multi-target server, like:
//
//	caddr=1.2.3.4:100 -. raddr=8.8.8.8:53
//	                  -. raddr=4.4.4.4:53
func handleUDP(ln net.PacketConn) {
	defer ln.Close()

	// sessMap
	sessMap := make(udpSessMap)
	defer func() {
		for _, v := range sessMap {
			v.Close()
		}
	}()

	buf := make([]byte, 1024*8)

	for {
		// 5 minutes timeout
		ln.SetReadDeadline(time.Now().Add(time.Minute * 5))

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

		// >>> REQ:
		//
		//	| RSV | FRAG | ATYP | DST.ADDR | DST.PORT | DATA |
		//	+-----+------+------+----------+----------+------+
		//	|  2  |  1   |  1   |    ...   |    2     |  ... |
		saddr, err := socks.ParseAddr(data[3:])
		if err != nil {
			continue
		}

		raddr := saddr.ToUDPAddr()
		data = data[3+len(saddr.B):]

		// dns cache query
		if raddr.Port == 53 {
			_, ans, erx := dns.OnRequest(data)
			if erx == nil && ans != nil {
				// dns cache answer
				if b := ans.Bytes(); len(b) > 0 {
					buf[0], buf[1], buf[2] = 0, 0, 0 // RSV|FRAG
					off := 3 + len(saddr.B)
					copy(buf[off:], b)
					ln.WriteTo(buf[:off+len(b)], caddr)
					continue
				}
			}
		}

		// lookfor or create sess
		var sess *udpSess_t

		// use source client addr as key
		// <TODO-Notice>
		//  some user's env has multi-outbound-ip, we will get diff caddr although he use one-same-conn
		key := nx.MakeKey(caddr)

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
		err = sess.WriteToRC(data, raddr)
		if err != nil {
			break
		}
	}
}
