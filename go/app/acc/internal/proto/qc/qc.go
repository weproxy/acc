//
// weproxy@foxmail.com 2022/11/02
//

package qc

import (
	"crypto/tls"
	"net"
	"net/url"
	"strings"

	"github.com/lucas-clemente/quic-go"

	"weproxy/acc/libgo/nx/stats"

	"weproxy/acc/app/acc/internal/proto"
)

const TAG = "[qc]"

func init() {
	proto.Register([]string{"qc", "quic"}, New)
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

// quicConn ...
type quicConn struct {
	quic.Stream
	c quic.Connection
}

// LocalAddr/RemoteAddr ...
func (m *quicConn) LocalAddr() net.Addr  { return m.c.LocalAddr() }
func (m *quicConn) RemoteAddr() net.Addr { return m.c.RemoteAddr() }

// dial ...
func (m *Handler) dial(flag uint8, raddr net.Addr, sta *stats.Stats) (rc net.Conn, err error) {
	tlsCfg := &tls.Config{
		InsecureSkipVerify: true,
		NextProtos:         []string{"weproxy-quic"}, // don't edit it, this string must same as server
	}

	c, err := quic.DialAddr(m.serv.String(), tlsCfg, nil)
	if err != nil {
		return
	}

	s, err := c.OpenStream()
	if err != nil {
		return
	}

	rc = &quicConn{c: c, Stream: s}

	sta.LogI("connected")

	buf := [4]byte{flag, 0, 0, 0}
	_, err = rc.Write(buf[:])
	if err != nil {
		rc.Close()
		return nil, err
	}

	return
}
