//
// weproxy@foxmail.com 2022/10/20
//

package s5

import (
	"net"
	"net/url"
	"strings"

	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/socks"
	"weproxy/acc/libgo/nx/stats"

	"weproxy/acc/app/acc/internal/conf"
	"weproxy/acc/app/acc/internal/proto"
)

const TAG = "[s5]"

func init() {
	proto.Register([]string{"s5", "s5x"}, New)
}

// New ...
func New(serv string) (proto.Handler, error) {
	var user, pass string
	saddr := serv
	if strings.Contains(serv, "://") {
		url, err := url.Parse(serv)
		if err != nil {
			return nil, err
		}

		saddr = url.Host
		if url.User != nil {
			user = url.User.Username()
			pass, _ = url.User.Password()
		}
	}
	raddr, err := net.ResolveUDPAddr("udp", saddr)
	if err != nil {
		return nil, err
	}

	return &Handler{serv: raddr, user: user, pass: pass}, nil
}

////////////////////////////////////////////////////////////////////////////////

// Handler implements proto.Handler
type Handler struct {
	user, pass string
	serv       *net.UDPAddr
}

// Close ...
func (m *Handler) Close() error {
	return nil
}

////////////////////////////////////////////////////////////////////////////////

// dial ...
func (m *Handler) dial(cmd socks.Command, caddr, raddr net.Addr, sta *stats.Stats) (rc net.Conn, bound *socks.Addr, err error) {
	rc, err = net.Dial("tcp", m.serv.String())
	if err != nil {
		return
	}

	sta.LogI("connected")

	// handshake
	bound, err = socks.ClientHandshake(rc, cmd, raddr, func() (user, pass string) {
		if len(m.user) == 0 {
			m.user, m.pass = conf.GetAuthS5(nx.GetIP(caddr).String())
		}
		if len(m.user) > 0 {
			return m.user, m.pass
		}
		return "user", "pass"
	})
	if err != nil {
		return
	}

	sta.LogI("handshaked, bound %v", bound)

	return
}
