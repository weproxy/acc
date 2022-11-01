//
// weproxy@foxmail.com 2022/10/20
//

package dns

import (
	"errors"
	"fmt"
	"net"
	"time"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/dns"
	"weproxy/acc/libgo/nx/netio"
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
func (m *udpSess_t) Start(target string) {
	tag := fmt.Sprintf("%s DNS_%v %v->%s", TAG, nx.NewID(), m.caddr, target)
	sta := stats.NewUDPStats(stats.TypeDNS, tag)

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
type udpSessMap map[nx.Key]*udpSess_t

////////////////////////////////////////////////////////////////////////////////

// udpConn_t
// to target server
type udpConn_t struct {
	net.PacketConn
	// typ socks.AddrType
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
	return m.PacketConn.WriteTo(buf, addr)
}

// //////////////////////////////////////////////////////////////////////////////

// runServLoop ...
func runServLoop(ln net.PacketConn) {
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

		logx.V("%s ReadFrom() %v", TAG, caddr)

		// packet data
		data := buf[:n]

		// dns cache query
		msg, ans, err := dns.OnRequest(data)
		if err == nil && ans != nil {
			// dns cache answer
			if b := ans.Bytes(); len(b) > 0 {
				ln.WriteTo(b, caddr)
				continue
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

			target := func() string {
				if msg != nil && len(msg.Questions) > 0 {
					return msg.Questions[0].Name.String() + msg.Questions[0].Type.String()
				}
				return ""
			}()

			// start recv loop...
			sess.Start(target)
		} else {
			sess = v
		}

		// target DNS server
		raddr := &net.UDPAddr{IP: net.IPv4(223, 5, 5, 5), Port: 53}

		// writeTo target server
		err = sess.WriteToRC(data, raddr)
		if err != nil {
			break
		}
	}
}
