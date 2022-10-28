//
// weproxy@foxmail.com 2022/10/20
//

package nx

import (
	"encoding/binary"
	"net"
	"sync/atomic"
)

////////////////////////////////////////////////////////////////////////////////

var (
	_id uint64
)

// NewID ...
func NewID() uint64 {
	return atomic.AddUint64(&_id, 1)
}

////////////////////////////////////////////////////////////////////////////////

// Key ...
type Key uint64

// MakeKey ...
func MakeKey(addr net.Addr) Key {
	switch addr := addr.(type) {
	case *net.TCPAddr:
		return MakeKeyIPPort(addr.IP, uint16(addr.Port))
	case *net.UDPAddr:
		return MakeKeyIPPort(addr.IP, uint16(addr.Port))
	}
	return 0
}

// MakeKeyIPPort ...
func MakeKeyIPPort(ip net.IP, port uint16) Key {
	a := binary.LittleEndian.Uint32(ip)
	b := uint32(port)
	return Key(a)<<16 | Key(b)
}

////////////////////////////////////////////////////////////////////////////////

// Packet ...
type Packet struct {
	Addr net.Addr
	Data []byte
}
