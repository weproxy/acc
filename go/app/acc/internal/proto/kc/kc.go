//
// weproxy@foxmail.com 2022/10/20
//

package kc

import (
	"net"
	"net/url"
	"strings"

	"github.com/xtaci/kcp-go"

	"weproxy/acc/libgo/nx/stats"

	"weproxy/acc/app/acc/internal/proto"
)

const TAG = "[kc]"

func init() {
	proto.Register([]string{"kc", "kcp"}, New)
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
func (m *Handler) dial(flag uint8, raddr net.Addr, sta *stats.Stats) (rc net.Conn, err error) {
	rc, err = kcp.Dial(m.serv.String())
	if err != nil {
		return
	}

	sta.LogI("connected")

	buf := [4]byte{flag, 0, 0, 0}
	_, err = rc.Write(buf[:])
	if err != nil {
		rc.Close()
		return nil, err
	}

	return
}
