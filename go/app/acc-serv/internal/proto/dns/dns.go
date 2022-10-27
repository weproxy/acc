//
// weproxy@foxmail.com 2022/10/20
//

package dns

import (
	"encoding/json"
	"errors"
	"net"

	"weproxy/acc/libgo/logx"

	"weproxy/acc/app/acc-serv/internal/proto"
)

// TAG ...
const TAG = "[dns]"

// init ...
func init() {
	proto.Register([]string{"dns"}, New)
}

////////////////////////////////////////////////////////////////////////////////

// New ...
func New(js json.RawMessage) (proto.Server, error) {
	var j struct {
		Listen string `json:"listen,omitempty"`
	}

	if err := json.Unmarshal(js, &j); err != nil {
		return nil, err
	}
	if len(j.Listen) == 0 {
		return nil, errors.New("invalid addr")
	}

	return &server_t{addr: j.Listen}, nil
}

// server_t ...
type server_t struct {
	addr string
	ln   net.PacketConn
}

// Start ...
func (m *server_t) Start() error {
	ln, err := net.ListenPacket("udp", m.addr)
	if err != nil {
		logx.E("%v Listen(%v), err: %v", TAG, m.addr, err)
		return err
	}

	logx.D("%v Start(%s)", TAG, m.addr)

	m.ln = ln
	go runServLoop(ln)

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
