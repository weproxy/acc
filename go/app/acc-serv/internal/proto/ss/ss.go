//
// weproxy@foxmail.com 2022/10/20
//

package ss

import (
	"encoding/json"
	"errors"
	"fmt"
	"net"
	"strings"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/socks"
	"weproxy/acc/libgo/nx/socks/shadow"

	"weproxy/acc/app/acc-serv/internal/proto"
)

// TAG ...
const TAG = "[ss]"

// init ...
func init() {
	proto.Register([]string{"ss", "ssx"}, New)
}

// New ...
func New(js json.RawMessage) (proto.Server, error) {
	var j struct {
		Listen string `json:"listen,omitempty"`
		Cipher string `json:"cipher,omitempty"`
	}

	if err := json.Unmarshal(js, &j); err != nil {
		return nil, err
	}
	if len(j.Listen) == 0 {
		return nil, errors.New("missing addr")
	}

	arr := strings.Split(j.Cipher, ":")
	if len(arr) != 2 || len(arr[0]) == 0 || len(arr[1]) == 0 {
		return nil, errors.New("missing cipher")
	}
	if _, err := shadow.NewCipher(arr[0], arr[1]); err != nil {
		return nil, fmt.Errorf("invalid cipher, %v", err)
	}

	return &server_t{addr: j.Listen, ciph: arr[0], pass: arr[1]}, nil
}

////////////////////////////////////////////////////////////////////////////////

// server_t ...
type server_t struct {
	addr       string
	ciph, pass string
	tcpln      net.Listener
	udpln      net.PacketConn
}

// Start ...
func (m *server_t) Start() error {
	tcpln, err := net.Listen("tcp", m.addr)
	if err != nil {
		logx.E("%v Listen(%v), err: %v", TAG, m.addr, err)
		return err
	}

	udpln, err := net.ListenPacket("udp", m.addr)
	if err != nil {
		tcpln.Close()
		logx.E("%v Listen(%v), err: %v", TAG, m.addr, err)
		return err
	}

	logx.D("%v Start(%s)", TAG, m.addr)

	// TCP
	m.tcpln = tcpln
	go func() {
		for {
			c, err := tcpln.Accept()
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

	// UDP
	m.udpln = udpln
	cipher, err := shadow.NewCipher(m.ciph, m.pass)
	if err != nil {
		return err
	}
	go handleUDP(cipher.NewPacketConn(udpln, false))

	return nil
}

// Close ...
func (m *server_t) Close() error {
	if m.udpln != nil {
		m.udpln.Close()
	}
	if m.tcpln != nil {
		m.tcpln.Close()
	}
	logx.D("%v Close()", TAG)
	return nil
}

////////////////////////////////////////////////////////////////////////////////

// handleConn ...
func (m *server_t) handleConn(c net.Conn) error {
	defer c.Close()

	cipher, err := shadow.NewCipher(m.ciph, m.pass)
	if err != nil {
		return err
	}

	cc := cipher.NewConn(c)

	_, raddr, err := socks.ReadAddr(cc)
	if err != nil {
		return err
	}

	return handleTCP(cc, raddr.ToTCPAddr())
}
