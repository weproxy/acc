//
// weproxy@foxmail.com 2022/10/20
//

package ss

import (
	"errors"
	"net"
	"net/url"
	"strings"
	"time"

	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/socks"
	"weproxy/acc/libgo/nx/socks/shadow"
	"weproxy/acc/libgo/nx/stats"

	"weproxy/acc/app/acc/internal/conf"
	"weproxy/acc/app/acc/internal/proto"
)

const TAG = "[ss]"

func init() {
	proto.Register([]string{"ss", "ssx"}, New)
}

// New ...
func New(serv string) (proto.Handler, error) {
	var ciph, pass string
	saddr := serv
	if strings.Contains(serv, "://") {
		url, err := url.Parse(serv)
		if err != nil {
			return nil, err
		}

		saddr = url.Host
		if url.User != nil {
			ciph = url.User.Username()
			pass, _ = url.User.Password()
		}
	}
	raddr, err := net.ResolveUDPAddr("udp", saddr)
	if err != nil {
		return nil, err
	}

	var cipher shadow.Cipher
	if len(ciph) > 0 && len(pass) > 0 {
		cipher, err = shadow.NewCipher(ciph, pass)
		if err != nil {
			return nil, err
		}
	}

	return &Handler{serv: raddr, cipher: cipher}, nil
}

////////////////////////////////////////////////////////////////////////////////

// Handler implements proto.Handler
type Handler struct {
	serv   *net.UDPAddr
	cipher shadow.Cipher
}

// Close ...
func (m *Handler) Close() error {
	return nil
}

// getCipher ...
func (m *Handler) getCipher(caddr net.Addr) (cipher shadow.Cipher, err error) {
	if m.cipher == nil {
		ciph, pass := conf.GetAuthSS(nx.GetIP(caddr).String())
		if len(ciph) > 0 && len(pass) > 0 {
			m.cipher, err = shadow.NewCipher(ciph, pass)
		} else {
			return nil, errors.New("missing cipher")
		}
	}
	return m.cipher, err
}

////////////////////////////////////////////////////////////////////////////////

// dial ...
func (m *Handler) dial(raddr net.Addr, sta *stats.Stats) (rc net.Conn, err error) {
	c, err := net.DialTimeout("tcp", m.serv.String(), time.Second*10)
	if err != nil {
		return
	}

	sta.LogI("connected")

	// >>> REQ:
	//
	//	| ATYP | DST.ADDR | DST.PORT |
	//	+------+----------+----------+
	//	|  1   |    ...   |    2     |
	dst := socks.FromNetAddr(raddr)
	buf := append([]byte(nil), dst.B...)

	rc = m.cipher.NewConn(c)
	if _, err = rc.Write(buf); err != nil {
		rc.Close()
		return
	}

	return
}
