//
// weproxy@foxmail.com 2022/11/02
//

package kc

import (
	"fmt"
	"time"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/stack/netstk"
	"weproxy/acc/libgo/nx/stats"
)

// Handle TCP ...
func (m *Handler) Handle(c netstk.Conn, head []byte) {
	caddr, raddr := c.LocalAddr(), c.RemoteAddr()

	tag := fmt.Sprintf("%s TCP_%v %v-%v->%s", TAG, c.ID(), caddr, m.serv, raddr)
	sta := stats.NewTCPStats(stats.TypeKCP, tag)

	sta.Start("connecting...")
	defer sta.Done("closed")

	// dial server
	rc, err := m.dial(0x01, raddr, sta)
	if err != nil {
		logx.E("%s err: %v", tag, err)
		return
	}
	defer rc.Close()

	var opt netio.RelayOption
	opt.B2A.ReadTimeout = time.Second * 60
	opt.ToB.Data = head
	opt.A2B.CopingFn = func(n int) { sta.AddSent(int64(n)) }
	opt.B2A.CopingFn = func(n int) { sta.AddRecv(int64(n)) }

	// Relay c <--> rc
	netio.Relay(c, rc, opt)
}
