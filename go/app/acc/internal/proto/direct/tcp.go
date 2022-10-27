//
// weproxy@foxmail.com 2022/10/20
//

package direct

import (
	"fmt"
	"net"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/stack/netstk"
	"weproxy/acc/libgo/nx/stats"
)

// Handle TCP ...
func (m *Handler) Handle(c netstk.Conn, head []byte) {
	caddr, raddr := c.LocalAddr(), c.RemoteAddr()

	tag := fmt.Sprintf("%s TCP_%v %v->%s", TAG, c.ID(), caddr, raddr)
	sta := stats.NewTCPStats(stats.TypeDirect, tag)

	sta.Start("connecting...")
	defer sta.Done("closed")

	rc, err := net.Dial("tcp", raddr.String())
	if err != nil {
		logx.E("%s err: %v", tag, err)
		return
	}
	defer rc.Close()

	sta.LogI("connected")

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
