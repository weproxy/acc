//
// weproxy@foxmail.com 2022/10/20
//

package dnat

import (
	"context"
	"io"
	"net"
	"syscall"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/stack/netstk"
	"weproxy/acc/libgo/nx/util"
)

const TAG = "[dnat]"

// New laddr 127.0.0.1:6666
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

	// loopAccept
	go m.loopAccept()

	return
}

// Close ...
func (m *Server) Close() error {
	if m.tcpln != nil {
		m.tcpln.Close()
	}
	return nil
}
