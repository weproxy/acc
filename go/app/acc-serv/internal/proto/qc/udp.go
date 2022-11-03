//
// weproxy@foxmail.com 2022/11/02
//

package qc

import (
	"fmt"
	"net"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/dns"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/socks"
	"weproxy/acc/libgo/nx/stats"
)

// udpLocal_t from source client
type udpLocal_t struct {
	net.Conn // a *quicConn
}

// ReadFrom source client
func (m *udpLocal_t) ReadFrom(p []byte) (n int, addr net.Addr, err error) {
	// >>> REQ:
	//
	//	| ATYP | DST.ADDR | DST.PORT | DATA |
	//	+------+----------+----------+------+
	//	|  1   |    ...   |    2     |  ... |

_retry:
	n, err = m.Conn.Read(p)
	if err != nil {
		return
	}

	raddr, err := socks.ParseAddr(p[:n])
	if err != nil {
		return
	}

	data := p[len(raddr.B):n]

	// dns cache query
	_, ans, err := dns.OnRequest(data)
	if err == nil && ans != nil {
		// dns cache answer
		if b := ans.Bytes(); len(b) > 0 {
			m.writeTo(true, b, raddr.ToUDPAddr())
			goto _retry
		}
	}

	n = copy(p, data)
	addr = raddr.ToUDPAddr()

	return
}

// WriteTo target server
func (m *udpLocal_t) WriteTo(b []byte, addr net.Addr) (n int, err error) {
	return m.writeTo(false, b, addr)
}

// writeTo source client
func (m *udpLocal_t) writeTo(isCacheAnswer bool, b []byte, addr net.Addr) (n int, err error) {
	// <<< RSP:
	//
	//	| ATYP | SRC.ADDR | SRC.PORT | DATA |
	//	+------+----------+----------+------+
	//	|  1   |    ...   |    2     |  ... |

	srcAddr := socks.FromNetAddr(addr)

	if !isCacheAnswer {
		if uaddr, ok := addr.(*net.UDPAddr); ok && uaddr.Port == 53 {
			dns.OnResponse(b)
		}
	}

	buf := append([]byte(nil), srcAddr.B...)
	if len(b) > 0 {
		buf = append(buf, b...)
	}

	_, err = m.Conn.Write(buf)
	if err == nil {
		n = len(b)
	}

	return
}

////////////////////////////////////////////////////////////////////////////////

// handleTCP ...
func (m *server_t) handleUDP(c net.Conn, raddr net.Addr) {
	tag := fmt.Sprintf("%s UDP_%v %v.%v", TAG, nx.NewID(), c.RemoteAddr(), raddr)
	sta := stats.NewTCPStats(stats.TypeQUIC, tag)

	sta.Start("connected")
	defer sta.Done("closed")

	// dial target server
	rc, err := net.ListenPacket("udp", ":0")
	if err != nil {
		logx.E("%s err: %v", TAG, err)
		return
	}
	defer rc.Close()

	ln := &udpLocal_t{Conn: c}

	opt := netio.RelayOption{}
	opt.A2B.CopingFn = func(n int) { sta.AddRecv(int64(n)) }
	opt.B2A.CopingFn = func(n int) { sta.AddSent(int64(n)) }

	// Relay ln <--> rc
	netio.RelayPacket(ln, rc, opt)
}
