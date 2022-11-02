//
// weproxy@foxmail.com 2022/11/02
//

package kc

import (
	"encoding/json"
	"errors"
	"io"
	"net"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/socks"

	"weproxy/acc/app/acc-serv/internal/proto"
)

// TAG ...
const TAG = "[kc]"

// init ...
func init() {
	proto.Register([]string{"kc", "kcp"}, New)
}

// New ...
func New(js json.RawMessage) (proto.Server, error) {
	var j struct {
		Listen string `json:"listen,omitempty"`
	}

	if err := json.Unmarshal(js, &j); err != nil {
		return nil, err
	}
	if len(j.Listen) == 0 {
		return nil, errors.New("missing addr")
	}

	return &server_t{addr: j.Listen}, nil
}

// server_t ...
type server_t struct {
	addr string
	ln   net.Listener
}

// Start ...
func (m *server_t) Start() error {
	ln, err := net.Listen("tcp", m.addr)
	if err != nil {
		logx.E("%v Listen(%v), err: %v", TAG, m.addr, err)
		return err
	}

	logx.D("%v Start(%s)", TAG, m.addr)

	m.ln = ln
	go func() {
		for {
			c, err := ln.Accept()
			if err != nil {
				if !errors.Is(err, net.ErrClosed) {
					logx.E("%v Accept(), err: %v", TAG, err)
				}
				break
			}

			logx.V("%v Accept() %v", TAG, c.RemoteAddr())

			go m.handleConn(c)
		}
	}()

	return nil
}

// Close ...
func (m *server_t) Close() error {
	if m.ln != nil {
		m.ln.Close()
	}
	logx.D("%v Close()", TAG)
	return nil
}

////////////////////////////////////////////////////////////////////////////////

// handleConn ...
func (m *server_t) handleConn(c net.Conn) {
	defer c.Close()

	var buf [16]byte

	_, err := io.ReadFull(c, buf[:4])
	if err != nil {
		logx.E("%v handleConn(), ReadHead err: %v", TAG, err)
		return
	}

	_, raddr, err := socks.ReadAddr(c)
	if err != nil {
		logx.E("%v handleConn(), ReadAddr err: %v", TAG, err)
		return
	}

	if buf[0]&0x01 != 0 {
		go m.handleTCP(c, raddr.ToTCPAddr())
	} else {
		go m.handleUDP(c, raddr.ToUDPAddr())
	}
}
