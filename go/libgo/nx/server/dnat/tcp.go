//
// weproxy@foxmail.com 2022/10/20
//

package dnat

import (
	"errors"
	"net"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/util"
)

////////////////////////////////////////////////////////////////////////////////

// stackConn implements netstk.Conn
type stackConn struct {
	*net.TCPConn
	id      uint64
	origDst net.Addr
}

// ID ...
func (m *stackConn) ID() uint64 {
	return m.id
}

// RemoteAddr ...
func (m *stackConn) RemoteAddr() net.Addr {
	return m.origDst
}

////////////////////////////////////////////////////////////////////////////////

// loopAccept ...
func (m *Server) loopAccept() {
	ln := m.tcpln.((*net.TCPListener))
	for {
		c, err := ln.AcceptTCP()
		if err != nil {
			if !errors.Is(err, net.ErrClosed) {
				logx.E("%s, ln.Accept(), err: ", TAG, err)
			}
			break
		}

		// get origin dst
		origDst, err := util.GetTCPOriginDst(c)
		if err != nil {
			logx.W("%s get TCP %v origin dst fail: %v", TAG, c.LocalAddr(), err)
			c.Close()
			continue
		}

		// handle it
		cc := &stackConn{
			TCPConn: c,
			id:      nx.NewID(),
			origDst: origDst,
		}
		go m.h.Handle(cc)
	}
}
