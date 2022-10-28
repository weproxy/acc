//
// weproxy@foxmail.com 2022/10/20
//

package server

import (
	"io"
	"runtime"
	"weproxy/acc/libgo/nx/stack/netstk"
)

const TAG = "[server]"

// New ...
func New(h netstk.Handler) *Server {
	return &Server{h: h}
}

////////////////////////////////////////////////////////////////////////////////

// Server ...
type Server struct {
	h       netstk.Handler
	closers []io.Closer
}

// Start ...
func (m *Server) Start() (err error) {
	if runtime.GOOS == "linux" {
		tproxy := &tproxyServer{h: m.h}
		if err = tproxy.Start("10.20.30.40:6666"); err != nil {
			return
		}
		m.closers = append(m.closers, tproxy)
	}

	return
}

// Close ...
func (m *Server) Close() error {
	for i := len(m.closers) - 1; i >= 0; i-- {
		m.closers[i].Close()
	}
	m.closers = nil
	return nil
}
