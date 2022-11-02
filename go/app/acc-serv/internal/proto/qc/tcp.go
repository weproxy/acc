//
// weproxy@foxmail.com 2022/11/02
//

package qc

import (
	"fmt"
	"net"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/socks"
	"weproxy/acc/libgo/nx/stats"
)

// handleTCP ...
func (m *server_t) handleTCP(c net.Conn, raddr net.Addr) {
	tag := fmt.Sprintf("%s TCP_%v %v.%v", TAG, nx.NewID(), c.RemoteAddr(), raddr)
	sta := stats.NewTCPStats(stats.TypeQUIC, tag)

	sta.Start("connected")
	defer sta.Done("closed")

	// dial target server
	rc, err := net.Dial("tcp", raddr.String())
	if err != nil {
		logx.E("%s dial, err: %v", TAG, err)
		socks.WriteReply(c, socks.ReplyHostUnreachable, 0, nil)
		return
	}
	rc.(*net.TCPConn).SetKeepAlive(true)
	defer rc.Close()

	opt := netio.RelayOption{}
	opt.A2B.CopingFn = func(n int) { sta.AddRecv(int64(n)) }
	opt.B2A.CopingFn = func(n int) { sta.AddSent(int64(n)) }

	// Relay c <--> rc
	netio.Relay(c, rc, opt)
}
