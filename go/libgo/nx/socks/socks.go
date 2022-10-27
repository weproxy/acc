//
// weproxy@foxmail.com 2022/10/20
//

package socks

import (
	"errors"
	"fmt"
	"io"
	"net"
	"time"
)

////////////////////////////////////////////////////////////////////////////////

// Version5 ...
const Version5 byte = 5

// UserAuthVersion ...
const UserAuthVersion byte = 0x01

////////////////////////////////////////////////////////////////////////////////

// Command ...
type Command byte

const (
	CmdConnect   Command = 0x01
	CmdBind      Command = 0x02
	CmdAssociate Command = 0x03
)

// String ...
func (m Command) String() string {
	return "Command"
}

// Method ...
type Method byte

const (
	AuthMethodNotRequired Method = 0x00 // no authentication required
	AuthMethodUserPass    Method = 0x02 // use username/password
)

// String ...
func (m Method) String() string {
	return "Method"
}

// Reply ...
type Reply byte

const (
	ReplyAuthSuccess Reply = 0
	ReplyAuthFailure Reply = 1

	ReplySuccess              Reply = 0
	ReplyGeneralFailure       Reply = 1
	ReplyConnectionNotAllowed Reply = 2
	ReplyNetworkUnreachable   Reply = 3
	ReplyHostUnreachable      Reply = 4
	ReplyConnectionRefused    Reply = 5
	ReplyTTLExpired           Reply = 6
	ReplyCommandNotSupported  Reply = 7
	ReplyAddressNotSupported  Reply = 8

	ReplyNoAcceptableMethods Reply = 0xff // no acceptable authentication methods
)

// String ...
func (m Reply) String() string {
	return "Reply"
}

// WriteReply ...
func WriteReply(w io.Writer, reply Reply, resverd byte, boundAddr net.Addr) error {
	// <<< REP:
	//     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
	//     +-----+-----+-------+------+----------+----------+
	//     |  1  |  1  | X'00' |  1   |    ...   |    2     |
	buf := make([]byte, 3+MaxAddrLen)
	cnt := 3

	buf[0] = Version5
	buf[1] = byte(reply)
	buf[2] = resverd

	if boundAddr != nil {
		if a := FromNetAddr(boundAddr); a != nil && len(a.B) > 0 {
			copy(buf[3:], a.B)
			cnt += len(a.B)
		}
	}

	_, err := w.Write(buf[0:cnt])

	return err
}

// ToError ...
func ToError(r Reply) error {
	return errors.New((r.String()))
}

// error ...
var (
	ErrUnrecognizedAddrType = errors.New("unrecognized address type")
	ErrInvalidSocksVersion  = errors.New("invalid socks version")
	ErrUserAuthFailed       = errors.New("user authentication failed")
	ErrNoSupportedAuth      = errors.New("no supported authentication mechanism")
)

////////////////////////////////////////////////////////////////////////////////

// ProvideUserPassFn ...
type ProvideUserPassFn func() (user, pass string)

// CheckUserPassFn ...
type CheckUserPassFn func(user, pass string) error

// clientAuth ...
func clientAuth(c io.ReadWriter, userPassRequired bool, provideUserPassFn ProvideUserPassFn) error {
	if !userPassRequired {
		return nil
	}

	user, pass, err := func() ([]byte, []byte, error) {
		if provideUserPassFn == nil {
			return nil, nil, errors.New("miss provideUserPassFn")
		}

		user, pass := provideUserPassFn()
		if len(user) == 0 || len(pass) == 0 {
			return nil, nil, errors.New("missing user/pass")
		}

		return []byte(user), []byte(pass), nil
	}()
	if err != nil {
		return nil
	}

	buf := make([]byte, 512)

	// >>> REQ:
	//     | VER | ULEN |  UNAME   | PLEN |  PASSWD  |
	//     +-----+------+----------+------+----------+
	//     |  1  |  1   | 1 to 255 |  1   | 1 to 255 |
	buf[0] = UserAuthVersion
	buf[1] = byte(len(user))
	copy(buf[2:], user)
	buf[2+len(user)] = byte(len(pass))
	copy(buf[2+len(user)+1:], pass)
	_, err = c.Write(buf[0 : 3+len(user)+len(pass)])
	if err != nil {
		return err
	}

	// <<< REP:
	//     | VER | STATUS |
	//     +-----+--------+
	//     |  1  |   1    |
	if _, err = io.ReadFull(c, buf[0:2]); err != nil {
		return err
	}
	if buf[0] != UserAuthVersion {
		return fmt.Errorf("invalid auth version: %d", buf[0])
	}
	if r := Reply(buf[1]); r != ReplyAuthSuccess {
		return ToError(r)
	}

	return nil
}

// serverAuth ...
func serverAuth(c io.ReadWriter, userPassRequired bool, checkUserPassFn CheckUserPassFn) error {
	buf := make([]byte, 512)

	// <<< REP:
	//     | VER | METHOD |
	//     +-----+--------+
	//     |  1  |   1    |
	buf[0] = Version5
	buf[1] = byte(AuthMethodNotRequired)
	if userPassRequired {
		buf[1] = byte(AuthMethodUserPass)
	}
	_, err := c.Write(buf[0:2])
	if err != nil {
		return err
	}

	if !userPassRequired {
		return nil
	}

	// >>> REQ:
	//     | VER | ULEN |  UNAME   | PLEN |  PASSWD  |
	//     +-----+------+----------+------+----------+
	//     |  1  |  1   | 1 to 255 |  1   | 1 to 255 |
	if _, err = io.ReadFull(c, buf[0:2]); err != nil {
		return err
	}
	if buf[0] != UserAuthVersion {
		return fmt.Errorf("invalid auth version: %d", buf[0])
	}

	userLen := buf[1]
	if int(userLen) > len(buf)-2 {
		return ErrUserAuthFailed
	}

	if _, err = io.ReadFull(c, buf[0:userLen+1]); err != nil {
		return err
	}

	user := string(buf[:userLen])

	passLen := buf[userLen]
	if int(passLen) > len(buf)-2 {
		return ErrUserAuthFailed
	}
	if _, err = io.ReadFull(c, buf[0:passLen]); err != nil {
		return err
	}
	pass := string(buf[:passLen])

	// check user pass
	if checkUserPassFn != nil {
		err = checkUserPassFn(user, pass)
	}

	// <<< REP:
	//     | VER | STATUS |
	//     +-----+--------+
	//     |  1  |   1    |
	buf[0] = UserAuthVersion
	buf[1] = byte(ReplyAuthSuccess)
	if err != nil {
		buf[1] = byte(ReplyAuthFailure)
	}

	_, err = c.Write(buf[0:2])

	return err
}

// ClientHandshake ...
func ClientHandshake(c net.Conn, cmd Command, addr net.Addr, provideUserPassFn ProvideUserPassFn) (bound *Addr, err error) {
	c.SetDeadline(time.Now().Add(time.Second * 5))
	defer c.SetDeadline(time.Time{})

	buf := make([]byte, 512)

	// >>> REQ:
	//     | VER | NMETHODS | METHODS  |
	//     +-----+----------+----------+
	//     |  1  |    1     | 1 to 255 |
	buf[0], buf[1], buf[2], buf[4] = Version5, 2, byte(AuthMethodNotRequired), byte(AuthMethodUserPass)
	if _, err = c.Write(buf[:2+int(buf[1])]); err != nil {
		return
	}

	// <<< REP:
	//     | VER | METHOD |
	//     +-----+--------+
	//     |  1  |   1    |
	_, err = io.ReadFull(c, buf[0:2])
	if err != nil {
		return
	}

	switch buf[1] {
	case byte(AuthMethodNotRequired):
	case byte(AuthMethodUserPass):
		// >>> REQ:
		//     | VER | ULEN |  UNAME   | PLEN |  PASSWD  |
		//     +-----+------+----------+------+----------+
		//     |  1  |  1   | 1 to 255 |  1   | 1 to 255 |

		// <<< REP:
		//     | VER | STATUS |
		//     +-----+--------+
		//     |  1  |   1    |
		err = clientAuth(c, true, provideUserPassFn)
	default:
		err = ErrNoSupportedAuth
	}
	if err != nil {
		return
	}

	// >>> REQ:
	//     | VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
	//     +-----+-----+-------+------+----------+----------+
	//     |  1  |  1  | X'00' |  1   |    ...   |    2     |
	saddr := FromNetAddr(addr)
	buf[0], buf[1], buf[2] = Version5, byte(cmd), 0
	copy(buf[3:], saddr.B)
	_, err = c.Write(buf[0 : 3+len(saddr.B)])
	if err != nil {
		return
	}

	// <<< REP:
	//     | VER | CMD |  RSV  | ATYP | BND.ADDR | BND.PORT |
	//     +-----+-----+-------+------+----------+----------+
	//     |  1  |  1  | X'00' |  1   |    ...   |    2     |
	_, err = io.ReadFull(c, buf[0:3])
	if err != nil {
		return
	}
	_, bound, err = ReadAddr(c)
	if err != nil {
		return
	}

	return
}

// ServerHandshake ...
func ServerHandshake(c net.Conn, checkUserPassFn CheckUserPassFn) (Command, *Addr, error) {
	c.SetDeadline(time.Now().Add(time.Second * 5))
	defer c.SetDeadline(time.Time{})

	cmd := Command(0)
	buf := make([]byte, 256)

	// >>> REQ:
	//     | VER | NMETHODS | METHODS  |
	//     +-----+----------+----------+
	//     |  1  |    1     | 1 to 255 |
	_, err := io.ReadFull(c, buf[0:2])
	if err != nil {
		WriteReply(c, ReplyAuthFailure, 0, nil)
		return cmd, nil, err
	}

	methodCnt := buf[1]

	if buf[0] != Version5 || methodCnt == 0 || methodCnt > 16 {
		WriteReply(c, ReplyAuthFailure, 0, nil)
		return cmd, nil, ErrInvalidSocksVersion
	}

	n, err := io.ReadFull(c, buf[0:methodCnt])
	if err != nil {
		WriteReply(c, ReplyAuthFailure, 0, nil)
		return cmd, nil, err
	}

	var methodNotRequired, methodUserPass bool
	for i := 0; i < n; i++ {
		switch Method(buf[i]) {
		case AuthMethodNotRequired:
			methodNotRequired = true
		case AuthMethodUserPass:
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
		err := serverAuth(c, methodUserPass, checkUserPassFn)
		if err != nil {
			return cmd, nil, err
		}
	} else {
		WriteReply(c, ReplyNoAcceptableMethods, 0, nil)
		return cmd, nil, ErrNoSupportedAuth
	}

	// >>> REQ:
	//     | VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
	//     +-----+-----+-------+------+----------+----------+
	//     |  1  |  1  | X'00' |  1   |    ...   |    2     |
	_, err = io.ReadFull(c, buf[0:3])
	if err != nil {
		WriteReply(c, ReplyAddressNotSupported, 0, nil)
		return cmd, nil, err
	}

	// ver := buf[0]
	cmd = Command(buf[1])
	// rsv := buf[2]

	if CmdConnect != cmd && CmdAssociate != cmd {
		WriteReply(c, ReplyCommandNotSupported, 0, nil)
		return cmd, nil, ToError(ReplyCommandNotSupported)
	}

	_, raddr, err := ReadAddr(c)
	if err != nil {
		WriteReply(c, ReplyAddressNotSupported, 0, nil)
		return cmd, nil, err
	}

	return cmd, raddr, nil
}
