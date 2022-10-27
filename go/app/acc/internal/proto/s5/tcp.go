//
// weproxy@foxmail.com 2022/10/20
//

package s5

import (
	"fmt"
	"net"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/socks"
	"weproxy/acc/libgo/nx/stack/netstk"
	"weproxy/acc/libgo/nx/stats"
)

// dial ...
func (m *Handler) dial(cmd socks.Command, serv, addr net.Addr, sta *stats.Stats) (rc net.Conn, bound *socks.Addr, err error) {
	rc, err = net.Dial("tcp", serv.String())
	if err != nil {
		return
	}

	sta.LogI("connected")

	// handshake
	bound, err = socks.ClientHandshake(rc, cmd, addr, func() (user, pass string) {
		return "user", "pass"
	})
	if err != nil {
		return
	}

	sta.LogI("handshaked, bound %v", bound)

	return
}

// Handle TCP ...
func (m *Handler) Handle(c netstk.Conn, head []byte) {
	caddr, raddr := c.LocalAddr(), c.RemoteAddr()

	tag := fmt.Sprintf("%s TCP_%v %v-%v->%s", TAG, c.ID(), caddr, m.serv, raddr)
	sta := stats.NewTCPStats(stats.TypeS5, tag)

	sta.Start("connecting...")
	defer sta.Done("closed")

	rc, bound, err := m.dial(socks.CmdConnect, m.serv, raddr, sta)
	if err != nil {
		logx.E("%s err: %v", tag, err)
		return
	}
	_ = bound
	defer rc.Close()

	var opt netio.RelayOption
	// opt.B2A.ReadTimeout = time.Second * 180
	opt.ToB.Data = head
	opt.A2B.CopingFn = func(n int) { sta.AddSent(int64(n)) }
	opt.B2A.CopingFn = func(n int) { sta.AddRecv(int64(n)) }

	// Relay c <--> rc
	err = netio.Relay(c, rc, opt)
	// if err != nil {
	// 	if !errors.Is(err, net.ErrClosed) {
	// 		logx.E("%s relay err: %v", tag, err)
	// 	}
	// }
}
