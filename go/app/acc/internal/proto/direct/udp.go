//
// weproxy@foxmail.com 2022/10/20
//

package direct

import (
	"encoding/binary"
	"fmt"
	"net"
	"time"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/dns"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/stack/netstk"
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
type udpSess_t struct {
	h             *Handler
	ln            netstk.PacketConn // to source client, it is netstk.PacketConn
	rc            net.PacketConn    // to target server, it is udpConn_t
	caddr         net.Addr          // source client addr
	raddr         net.Addr          // target server addr(maybe changed after)
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
	tag := fmt.Sprintf("%s UDP_%v %v->%s", TAG, m.ln.ID(), m.caddr, target)
	sta := stats.NewUDPStats(stats.TypeDirect, tag)

	sta.Start("started")
	m.sta = sta

	// loopRecvRC
	go func() {
		defer m.Close()

		var opt netio.CopyOption
		opt.WriteAddr = m.raddr
		opt.ReadTimeout = time.Second * 60
		opt.CopingFn = func(n int) { m.sta.AddRecv(int64(n)) }

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
	m.sta.AddSent(int64(len(buf)))

	// udpConn_t.WriteTo()
	// unpack data (from source client), and writeTo target server
	if m.h != nil && m.h.serv != nil {
		raddr = m.h.serv
	}
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
	raddr net.Addr
}

// ReadFrom override
// readFrom target server, will writeTo source client
func (m *udpConn_t) ReadFrom(buf []byte) (n int, addr net.Addr, err error) {
	n, addr, err = m.PacketConn.ReadFrom(buf)
	if err == nil {
		// dns cache store
		dns.OnResponse(buf[:n])
	}
	return
}

// WriteTo override
// data from source client writeTo target server
func (m *udpConn_t) WriteTo(buf []byte, addr net.Addr) (n int, err error) {
	if m.raddr != nil {
		addr = m.raddr
	}
	return m.PacketConn.WriteTo(buf, addr)
}

////////////////////////////////////////////////////////////////////////////////

// _udpSessMap
var _udpSessMap = make(udpSessMap)

// getUDPSess lookfor or create sess
func (m *Handler) getUDPSess(pc netstk.PacketConn, caddr, raddr net.Addr) (sess *udpSess_t) {
	// use source client addr as key
	key := makeKey(caddr)

	if v, ok := _udpSessMap[key]; !ok {
		// create a new rc for every new source client
		c, err := net.ListenPacket("udp", ":0")
		if err != nil {
			logx.E("%s err: %v", TAG, err)
			return
		}

		// wrap as udpConn_t
		rc := &udpConn_t{PacketConn: c}

		// store to _udpSessMap
		sess = &udpSess_t{
			h:     m,
			ln:    pc,
			rc:    rc,
			caddr: caddr,
			raddr: raddr,
		}
		_udpSessMap[key] = sess

		// remove it after rc closed
		sess.afterClosedFn = func() { delete(_udpSessMap, key) }

		// start recv loop...
		if m.serv != nil {
			raddr = m.serv
		}
		sess.Start(raddr.String())
	} else {
		sess = v
	}

	return sess
}
