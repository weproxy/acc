//
// weproxy@foxmail.com 2022/10/20
//

package tproxy

import (
	"context"
	"io"
	"net"
	"syscall"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/stack/netstk"
	"weproxy/acc/libgo/nx/util"
)

const TAG = "[tproxy]"

// New laddr 10.20.30.40:5000
func New(h netstk.Handler, laddr string) (io.Closer, error) {
	m := &Server{h: h, laddr: laddr}
	if err := m.Start(); err != nil {
		return nil, err
	}
	return m, nil
}

////////////////////////////////////////////////////////////////////////////////

// Server ...
type Server struct {
	h     netstk.Handler
	laddr string
	tcpln net.Listener
	udpln net.PacketConn
}

// Start ...
func (m *Server) Start() (err error) {
	lc := net.ListenConfig{
		Control: func(network, address string, c syscall.RawConn) error {
			return c.Control(func(fd uintptr) {
				util.SetOptTransparent(int(fd), 1)
				util.SetOptRecvOrigDst(int(fd), 1)
			})
		},
	}

	laddr := m.laddr

	// TCP
	m.tcpln, err = lc.Listen(context.Background(), "tcp", laddr)
	if err != nil {
		logx.E("%s TCP listen on: %v, %v", TAG, laddr, err)
		return
	}
	logx.I("%s TCP start on: %v", TAG, laddr)

	// UDP
	m.udpln, err = lc.ListenPacket(context.Background(), "udp", laddr)
	if err != nil {
		logx.E("%s UDP listen on: %v, %v", TAG, laddr, err)
		return
	}
	logx.I("%s UDP start on: %v", TAG, laddr)

	// loopAccept
	go m.loopAccept()

	// loopRecvFrom
	go m.loopRecvFrom()

	return
}

// Close ...
func (m *Server) Close() error {
	if m.udpln != nil {
		m.udpln.Close()
	}
	if m.tcpln != nil {
		m.tcpln.Close()
	}
	return nil
}
