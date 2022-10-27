//
// weproxy@foxmail.com 2022/10/20
//

package s5

import (
	"fmt"
	"io"
	"net"
	"time"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/dns"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/socks"
	"weproxy/acc/libgo/nx/stack/netstk"
	"weproxy/acc/libgo/nx/stats"

	"weproxy/acc/app/acc/internal/proto/rule"
)

////////////////////////////////////////////////////////////////////////////////

// udpLocal_t ...
type udpLocal_t struct {
	net.PacketConn
	raddr net.Addr
}

// ReadFrom override
// readFrom source client, will writeTo target server
func (m *udpLocal_t) ReadFrom(buf []byte) (n int, addr net.Addr, err error) {
	// >>> REQ:
	//
	//	| RSV | FRAG | ATYP | DST.ADDR | DST.PORT | DATA |
	//	+-----+------+------+----------+----------+------+
	//	|  2  |  1   |  1   |    ...   |    2     |  ... |

	buf[0], buf[1], buf[2] = 0, 0, 0
	saddr := socks.FromNetAddr(addr)
	copy(buf[3:], saddr.B)

	off := 3 + len(saddr.B)
	n, addr, err = m.PacketConn.ReadFrom(buf[off:])
	if err != nil {
		return
	}
	n += off

	return
}

// WriteTo override
// data from target server writeTo source client
func (m *udpLocal_t) WriteTo(buf []byte, addr net.Addr) (n int, err error) {
	// <<< RSP:
	//
	//	| RSV | FRAG | ATYP | SRC.ADDR | SRC.PORT | DATA |
	//	+-----+------+------+----------+----------+------+
	//	|  2  |  1   |  1   |    ...   |    2     |  ... |

	saddr, err := socks.ParseAddr(buf[3:])
	if err != nil {
		return
	}

	data := buf[3+len(saddr.B):]

	// dns cache store
	dns.OnResponse(data)

	return m.PacketConn.WriteTo(data, saddr.ToUDPAddr())
}

////////////////////////////////////////////////////////////////////////////////

// HandlePacket ...
func (m *Handler) HandlePacket(pc netstk.PacketConn, head []byte) {
	caddr, raddr := pc.LocalAddr(), pc.RemoteAddr()

	tag := fmt.Sprintf("%s Assoc_%v %v-%v->%s", TAG, pc.ID(), caddr, m.serv, raddr)
	sta := stats.NewTCPStats(stats.TypeS5, tag)

	sta.Start("connecting...")
	defer sta.Done("closed")

	// dial server
	rc, bound, err := m.dial(socks.CmdAssociate, raddr, sta)
	if err != nil {
		logx.E("%s err: %v", tag, err)
		return
	}
	defer rc.Close()

	go func() {
		// Copy io.Discard <-- rc
		_, err = io.Copy(io.Discard, rc)
		// if err != nil {
		// 	if !errors.Is(err, net.ErrClosed) {
		// 		logx.E("%s relay err: %v", tag, err)
		// 	}
		// }
	}()

	// handlePacket ...
	m.handlePacket(pc, bound, head)
}

// handlePacket ...
func (m *Handler) handlePacket(pc netstk.PacketConn, bound *socks.Addr, head []byte) {
	caddr, raddr := pc.LocalAddr(), pc.RemoteAddr()
	serv := bound.ToUDPAddr()

	tag := fmt.Sprintf("%s UDP_%v %v->%v->%v", TAG, pc.ID(), caddr, serv, raddr)
	sta := stats.NewUDPStats(stats.TypeS5, tag)

	sta.Start("connected")
	defer sta.Done("closed")

	rc, err := net.ListenPacket("udp", ":0")
	if err != nil {
		logx.E("%s err: %v", tag, err)
		return
	}
	defer rc.Close()

	ln := &udpLocal_t{PacketConn: pc, raddr: m.serv}

	var opt netio.RelayOption
	opt.B2A.ReadTimeout = time.Second * 180
	opt.ToB.Data = head
	opt.ToB.Addr = raddr
	opt.A2B.CopingFn = func(n int) { sta.AddSent(int64(n)) }
	opt.B2A.CopingFn = func(n int) { sta.AddRecv(int64(n)) }

	if m.serv != nil {
		opt.B2A.WriteAddr = raddr
	}

	if rule.IsDNSAddr(raddr) {
		opt.B2A.MaxTimes = 1
		opt.B2A.ReadTimeout = time.Second * 10
	}

	// RelayPacket ln <--> rc
	err = netio.RelayPacket(ln, rc, opt)
	// if err != nil {
	// 	if !errors.Is(err, net.ErrClosed) {
	// 		logx.E("%s relay err: %v", tag, err)
	// 	}
	// }
}
