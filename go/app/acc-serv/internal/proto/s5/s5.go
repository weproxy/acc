//
// weproxy@foxmail.com 2022/10/20
//

package s5

import (
	"encoding/json"
	"errors"
	"net"
	"strings"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/socks"

	"weproxy/acc/app/acc-serv/internal/proto"
)

// TAG ...
const TAG = "[s5]"

// init ...
func init() {
	proto.Register([]string{"s5", "s5x"}, New)
}

////////////////////////////////////////////////////////////////////////////////

// New ...
func New(js json.RawMessage) (proto.Server, error) {
	var j struct {
		Listen string   `json:"listen,omitempty"`
		Auth   []string `json:"auth,omitempty"`
	}

	if err := json.Unmarshal(js, &j); err != nil {
		return nil, err
	}
	if len(j.Listen) == 0 {
		return nil, errors.New("missing addr")
	}
	if len(j.Auth) == 0 {
		return nil, errors.New("missing auth")
	}

	auth := make(map[string]string)
	for _, ss := range auth {
		arr := strings.Split(ss, ":")
		if len(arr) == 2 && len(arr[0]) > 0 && len(arr[1]) > 0 {
			auth[arr[0]] = arr[1]
		}
	}

	return &server_t{addr: j.Listen, auth: auth}, nil
}

////////////////////////////////////////////////////////////////////////////////

// server_t ...
type server_t struct {
	addr string
	auth map[string]string
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

// cehckUserPass ...
func (m *server_t) cehckUserPass(user, pass string) error {
	if v, ok := m.auth[user]; ok && v == pass {
		return nil
	}
	return errors.New("invalid user/pass")
}

////////////////////////////////////////////////////////////////////////////////

// handleConn ...
func (m *server_t) handleConn(c net.Conn) {
	defer c.Close()

	cmd, raddr, err := socks.ServerHandshake(c, m.cehckUserPass)
	if err != nil || raddr == nil {
		logx.E("%v handshake(), err: %v", TAG, err)
		return
	}

	switch cmd {
	case socks.CmdConnect:
		handleTCP(c, raddr.ToTCPAddr())
	case socks.CmdAssoc:
		handleAssoc(c, raddr.ToUDPAddr())
	case socks.CmdBind:
		logx.E("%v not support socks command: bind", TAG)
	default:
		logx.E("%v unknow socks command: %d", TAG, cmd)
	}
}
