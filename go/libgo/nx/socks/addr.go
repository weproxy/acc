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

// MaxAddrLen ...
const MaxAddrLen = 1 + 1 + 255 + 2

var (
	ErrInvalidAddrType = errors.New("invalid address type")
	ErrInvalidAddrLen  = errors.New("invalid address length")
)

// AddrType ...
type AddrType byte

const (
	AddrTypeIPv4   AddrType = 1
	AddrTypeDomain AddrType = 3
	AddrTypeIPv6   AddrType = 4
)

// String ...
func (m AddrType) String() string {
	return "AddrType"
}

// Addr ...
type Addr struct {
	B []byte
}

// String ...
func (m *Addr) String() string {
	if m == nil || len(m.B) == 0 {
		return "<nil>"
	}
	return fmt.Sprintf("%v:%d", m.IP(), m.Port())
}

// IP ...
func (m *Addr) IP() net.IP {
	if m != nil && len(m.B) > 0 {
		switch AddrType(m.B[0]) {
		case AddrTypeIPv4:
			return net.IP(m.B[1 : 1+net.IPv4len])
		case AddrTypeIPv6:
			return net.IP(m.B[1 : 1+net.IPv6len])
		}
	}
	return net.IP{}
}

// Port ...
func (m *Addr) Port() int {
	if m != nil && len(m.B) > 0 {
		switch AddrType(m.B[0]) {
		case AddrTypeIPv4:
			return int(m.B[1+net.IPv4len])<<8 | int(m.B[1+net.IPv4len+1])
		case AddrTypeIPv6:
			return int(m.B[1+net.IPv6len])<<8 | int(m.B[1+net.IPv6len+1])
		case AddrTypeDomain:
			return int(m.B[2+m.B[1]])<<8 | int(m.B[2+m.B[1]+1])
		}
	}
	return 0
}

// ToTCPAddr ...
func (m *Addr) ToTCPAddr() *net.TCPAddr {
	if m == nil || len(m.B) == 0 {
		return nil
	}
	return &net.TCPAddr{IP: m.IP(), Port: m.Port()}
}

// ToUDPAddr ...
func (m *Addr) ToUDPAddr() *net.UDPAddr {
	if m == nil || len(m.B) == 0 {
		return nil
	}
	return &net.UDPAddr{IP: m.IP(), Port: m.Port()}
}

// fromNetAddr ...
func fromNetAddr(addr net.Addr) (ip net.IP, port int) {
	if addr != nil {
		switch addr := addr.(type) {
		case *net.TCPAddr:
			return addr.IP, addr.Port
		case *net.UDPAddr:
			return addr.IP, addr.Port
		}
	}
	return nil, 0
}

// FromNetAddr ...
func (m *Addr) FromNetAddr(addr net.Addr) {
	if ip, port := fromNetAddr(addr); ip != nil {
		if ip4 := ip.To4(); ip4 != nil {
			B := make([]byte, 1+net.IPv4len+2)
			B[0] = byte(AddrTypeIPv4)
			copy(B[1:], ip4[:net.IPv4len])
			B[1+net.IPv4len] = uint8(port >> 8)
			B[1+net.IPv4len+1] = uint8(port)
			m.B = B
		} else if ip6 := ip.To16(); ip6 != nil {
			B := make([]byte, 1+net.IPv6len+2)
			B[0] = byte(AddrTypeIPv6)
			copy(B[1:], ip6[:net.IPv6len])
			B[1+net.IPv6len] = uint8(port >> 8)
			B[1+net.IPv6len+1] = uint8(port)
			m.B = B
		}
	}
}

// FromNetAddr ...
func FromNetAddr(addr net.Addr) *Addr {
	a := &Addr{}
	a.FromNetAddr(addr)
	return a
}

// //////////////////////////////////////////////////////////////////////////////
// ParseAddr ...
//
//	| ATYP | ADDR | PORT |
//	+------+------+------+
//	|  1   |  x   |  2   |
func ParseAddr(B []byte) (*Addr, error) {
	if len(B) < 1+1+1+2 {
		return nil, ErrInvalidAddrLen
	}

	m := byte(0)
	if byte(AddrTypeIPv4) == B[0] {
		m = 1 + net.IPv4len + 2
	} else if byte(AddrTypeIPv6) == B[0] {
		m = 1 + net.IPv6len + 2
	} else if byte(AddrTypeDomain) == B[0] {
		m = 1 + 1 + B[1] + 2 // DOMAIN_LEN = B[1]
	} else {
		return nil, ErrInvalidAddrLen
	}

	if len(B) < int(m) {
		return nil, ErrInvalidAddrLen
	}

	return &Addr{B: B[0:m]}, nil
}

// ReadAddr ...
//
//	| ATYP | ADDR | PORT |
//	+------+------+------+
//	|  1   |  x   |  2   |
func ReadAddr(r io.Reader) (n int, addr *Addr, err error) {
	B := make([]byte, MaxAddrLen)

	// 2bytes = ATYP + (MAYBE)DOMAIN_LEN
	n, err = io.ReadFull(r, B[0:2])
	if err != nil {
		return
	}

	m := 0
	if byte(AddrTypeIPv4) == B[0] {
		m = 1 + net.IPv4len + 2
	} else if byte(AddrTypeIPv6) == B[0] {
		m = 1 + net.IPv6len + 2
	} else if byte(AddrTypeDomain) == B[0] {
		m = 1 + 1 + int(B[1]) + 2 // DOMAIN_LEN = B[1]
	} else {
		return n, nil, ErrInvalidAddrLen
	}

	t, err := io.ReadFull(r, B[2:m])
	if t > 0 {
		n += t
	}
	if err != nil {
		return
	}

	return n, &Addr{B: B[0:m]}, nil
}
