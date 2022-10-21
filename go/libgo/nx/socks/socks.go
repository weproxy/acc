//
// weproxy@foxmail.com 2022/10/20
//

package socks

import (
	"errors"
	"fmt"
	"io"
	"net"
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

// CheckUserPassFn ...
type CheckUserPassFn func(user, pass string) error

// ServerAuth ...
func ServerAuth(c io.ReadWriter, userPassRequired bool, checkUserPassFn CheckUserPassFn) error {
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
