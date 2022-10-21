//
// weproxy@foxmail.com 2022/10/20
//

package s5

import (
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"time"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/socks"

	"weproxy/acc/app/acc-serv/internal/proto"
)

// TAG ...
const TAG = "[s5]"

// init ...
func init() {
	proto.Register("s5", New)
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

////////////////////////////////////////////////////////////////////////////////

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
				if err != net.ErrClosed {
					logx.E("%v Accept(), err: %v", TAG, err)
				}
				break
			}

			logx.V("%v Accept() %v", TAG, c.RemoteAddr())

			go handleConn(c)
		}
	}()

	return nil
}

// Close ...
func (m *server_t) Close() error {
	logx.D("%v Close()", TAG)
	return nil
}

////////////////////////////////////////////////////////////////////////////////

// handleConn ...
func handleConn(c net.Conn) error {
	defer c.Close()

	cmd, raddr, err := handshake(c)
	if err != nil || raddr == nil {
		logx.E("%v handshake(), err: %v", TAG, err)
		return err
	}

	switch cmd {
	case socks.CmdConnect:
		return handleTCP(c, raddr.ToTCPAddr())
	case socks.CmdAssociate:
		return handleAssoc(c, raddr.ToUDPAddr())
	case socks.CmdBind:
		return errors.New("not support socks command: bind")
	default:
		return fmt.Errorf("unknow socks command: %d", cmd)
	}
}

////////////////////////////////////////////////////////////////////////////////

// checkUserPass ...
func checkUserPass(user, pass string) error {
	return nil
}

// handshake ...
func handshake(c net.Conn) (socks.Command, *socks.Addr, error) {
	c.SetDeadline(time.Now().Add(time.Second * 5))
	defer c.SetDeadline(time.Time{})

	cmd := socks.Command(0)
	buf := make([]byte, 256)

	// >>> REQ:
	//     | VER | NMETHODS | METHODS  |
	//     +-----+----------+----------+
	//     |  1  |    1     | 1 to 255 |

	_, err := io.ReadFull(c, buf[0:2])
	if err != nil {
		socks.WriteReply(c, socks.ReplyAuthFailure, 0, nil)
		return cmd, nil, err
	}

	methodCnt := buf[1]

	if buf[0] != socks.Version5 || methodCnt == 0 || methodCnt > 16 {
		socks.WriteReply(c, socks.ReplyAuthFailure, 0, nil)
		return cmd, nil, socks.ErrInvalidSocksVersion
	}

	n, err := io.ReadFull(c, buf[0:methodCnt])
	if err != nil {
		socks.WriteReply(c, socks.ReplyAuthFailure, 0, nil)
		return cmd, nil, err
	}

	var methodNotRequired, methodUserPass bool
	for i := 0; i < n; i++ {
		switch socks.Method(buf[i]) {
		case socks.AuthMethodNotRequired:
			methodNotRequired = true
		case socks.AuthMethodUserPass:
			methodUserPass = true
		}
	}

	if methodNotRequired || methodUserPass {
		// <<< REP:
		//     | VER | METHOD |
		//     +-----+--------+
		//     |  1  |   1    |

		// >>> REQ:
		//     | VER | ULEN |  UNAME   | PLEN |  PASSWD  |
		//     +-----+------+----------+------+----------+
		//     |  1  |  1   | 1 to 255 |  1   | 1 to 255 |

		// <<< REP:
		//     | VER | STATUS |
		//     +-----+--------+
		//     |  1  |   1    |
		err := socks.ServerAuth(c, methodUserPass, checkUserPass)
		if err != nil {
			return cmd, nil, err
		}
	} else {
		socks.WriteReply(c, socks.ReplyNoAcceptableMethods, 0, nil)
		return cmd, nil, socks.ErrNoSupportedAuth
	}

	// >>> REQ:
	//     | VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
	//     +-----+-----+-------+------+----------+----------+
	//     |  1  |  1  | X'00' |  1   |    ...   |    2     |

	_, err = io.ReadFull(c, buf[0:3])
	if err != nil {
		socks.WriteReply(c, socks.ReplyAddressNotSupported, 0, nil)
		return cmd, nil, err
	}

	// ver := buf[0]
	cmd = socks.Command(buf[1])
	// rsv := buf[2]

	if socks.CmdConnect != cmd && socks.CmdAssociate != cmd {
		socks.WriteReply(c, socks.ReplyCommandNotSupported, 0, nil)
		return cmd, nil, socks.ToError(socks.ReplyCommandNotSupported)
	}

	_, raddr, err := socks.ReadAddr(c)
	if err != nil {
		socks.WriteReply(c, socks.ReplyAddressNotSupported, 0, nil)
		return cmd, nil, err
	}

	return cmd, raddr, nil
}
