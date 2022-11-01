//
// weproxy@foxmail.com 2022/10/20
//

package direct

import (
	"net"
	"net/url"
	"strings"

	"weproxy/acc/app/acc/internal/proto"
)

const TAG = "[direct]"

func init() {
	proto.Register([]string{"di", "direct", "dns"}, New)
}

// New ...
func New(serv string) (proto.Handler, error) {
	saddr := serv
	if strings.Contains(serv, "://") {
		if url, err := url.Parse(serv); err != nil {
			return nil, err
		} else {
			saddr = url.Host
		}
	}
	raddr, err := net.ResolveUDPAddr("udp", saddr)
	if err != nil {
		return nil, err
	}

	return &Handler{serv: raddr}, nil
}

////////////////////////////////////////////////////////////////////////////////

// Handler implements proto.Handler
type Handler struct {
	serv *net.UDPAddr
}

// Close ...
func (m *Handler) Close() error {
	return nil
}
