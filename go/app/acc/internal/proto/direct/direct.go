//
// weproxy@foxmail.com 2022/10/20
//

package direct

import (
	"net"
	"net/url"
	"strconv"
	"strings"

	"weproxy/acc/app/acc/internal/proto"
)

const TAG = "[direct]"

func init() {
	proto.Register([]string{"di", "direct", "dns"}, New)
}

// New ...
func New(serv string) (proto.Handler, error) {
	var raddr net.Addr
	if strings.Contains(serv, "://") {
		if url, err := url.Parse(serv); err == nil {
			if host, port, err := net.SplitHostPort(url.Host); err == nil {
				if ip := net.ParseIP(host); ip != nil {
					var pn int
					if n, err := strconv.Atoi(port); err == nil {
						pn = n
					}
					raddr = &net.UDPAddr{IP: ip, Port: pn}
				}
			}
		}
	}

	return &Handler{serv: raddr}, nil
}

////////////////////////////////////////////////////////////////////////////////

// Handler implements proto.Handler
type Handler struct {
	serv net.Addr
}

// Close ...
func (m *Handler) Close() error {
	return nil
}
