//
// weproxy@foxmail.com 2022/10/20
//

package direct

import (
	"fmt"
	"net"
	"time"
	"weproxy/acc/app/acc/internal/proto/rule"
	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/dns"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/stack/netstk"
	"weproxy/acc/libgo/nx/stats"
)

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

// HandlePacket ...
func (m *Handler) HandlePacket(pc netstk.PacketConn, head []byte) {
	caddr, raddr := pc.LocalAddr(), pc.RemoteAddr()
	serv := raddr
	if m.serv != nil {
		serv = m.serv
	}

	tag := fmt.Sprintf("%s UDP_%v %v->%s", TAG, pc.ID(), caddr, serv)
	sta := stats.NewUDPStats(stats.TypeDirect, tag)

	sta.Start("connected...")
	defer sta.Done("closed")

	c, err := net.ListenPacket("udp", ":0")
	if err != nil {
		logx.E("%s err: %v", tag, err)
		return
	}
	defer c.Close()

	rc := &udpConn_t{PacketConn: c, raddr: m.serv}

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

	// RelayPacket c <--> rc
	err = netio.RelayPacket(pc, rc, opt)
	// if err != nil {
	// 	if !errors.Is(err, net.ErrClosed) {
	// 		logx.E("%s relay err: %v", tag, err)
	// 	}
	// }
}
