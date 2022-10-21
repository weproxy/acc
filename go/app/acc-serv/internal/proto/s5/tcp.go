//
// weproxy@foxmail.com 2022/10/20
//

package s5

import (
	"errors"
	"fmt"
	"io"
	"net"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/netio"
	"weproxy/acc/libgo/nx/socks"
	"weproxy/acc/libgo/nx/stats"
)

////////////////////////////////////////////////////////////////////////////////

// handleTCP ...
func handleTCP(c net.Conn, raddr net.Addr) error {
	tag := fmt.Sprintf("%s TCP_%v %v.%v", TAG, nx.NewID(), c.RemoteAddr(), raddr)
	sta := stats.NewTCPStats(stats.TypeS5, tag)

	sta.Start("connected")
	defer sta.Done("closed")

	// dial target server
	rc, err := net.Dial("tcp", raddr.String())
	if err != nil {
		logx.E("%s dial, err: %v", TAG, err)
		socks.WriteReply(c, socks.ReplyHostUnreachable, 0, nil)
		return err
	}
	defer rc.Close()

	// <<< REP:
	//     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
	//     +-----+-----+-------+------+----------+----------+
	//     |  1  |  1  | X'00' |  1   |    ...   |    2     |
	err = socks.WriteReply(c, socks.ReplySuccess, 0, &net.TCPAddr{IP: net.IPv4zero, Port: 0})
	if err != nil {
		if !errors.Is(err, net.ErrClosed) {
			logx.E("%s err: ", TAG, err)
		}
		return err
	}

	opt := netio.RelayOption{}
	opt.A2B.CopingFn = func(n int) { sta.AddRecv(int64(n)) }
	opt.B2A.CopingFn = func(n int) { sta.AddSent(int64(n)) }

	// Relay c <-. rc
	err = netio.Relay(c, rc, opt)
	if err != nil {
		if !errors.Is(err, net.ErrClosed) {
			logx.E("%s relay %s, err: %v", TAG, tag, err)
		}
	}

	return err
}

////////////////////////////////////////////////////////////////////////////////

// handleAssoc ...
func handleAssoc(c net.Conn, raddr net.Addr) error {
	caddr := c.RemoteAddr()

	tag := fmt.Sprintf("%s Assoc_%v %v.%v", TAG, nx.NewID(), caddr, raddr)
	sta := stats.NewTCPStats(stats.TypeS5, tag)

	sta.Start("connected")
	defer sta.Done("closed")

	ln, err := net.ListenPacket("udp", ":0")
	if err != nil {
		logx.E("%s dial, err: %v", TAG, err)
		socks.WriteReply(c, socks.ReplyHostUnreachable, 0, nil)
		return err
	}

	// handleUDP
	// raddr is not fixed, it maybe be changed after
	go handleUDP(ln, caddr, raddr)

	// <<< REP:
	//     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
	//     +-----+-----+-------+------+----------+----------+
	//     |  1  |  1  | X'00' |  1   |    ...   |    2     |
	err = socks.WriteReply(c, socks.ReplySuccess, 0, ln.LocalAddr())
	if err != nil {
		if !errors.Is(err, net.ErrClosed) {
			logx.E("%s err: %v", TAG, err)
		}
		return err
	}

	_, err = io.Copy(io.Discard, c)
	if err != nil {
		if !errors.Is(err, net.ErrClosed) {
			logx.E("%s err: %v", TAG, err)
		}
		return err
	}

	return nil
}
