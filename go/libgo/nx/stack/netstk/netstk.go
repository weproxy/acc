//
// weproxy@foxmail.com 2022/10/20
//

package netstk

import (
	"io"
	"net"
)

// Conn ...
type Conn interface {
	net.Conn
	ID() uint64
	CloseRead() error
	CloseWrite() error
}

// PacketConn ...
type PacketConn interface {
	net.PacketConn
	ID() uint64
	RemoteAddr() net.Addr
}

// Device ...
type Device interface {
	io.Closer

	// Read from device
	Read(p []byte, offset int) (n int, err error)

	// Write to device
	Write(p []byte, offset int) (n int, err error)

	// Type ...
	Type() string
}

// Handler ...
type Handler interface {
	io.Closer

	// Handle TCP ...
	Handle(Conn)

	// HandlePacket UDP ...
	HandlePacket(PacketConn)
}

// Stack ...
type Stack interface {
	io.Closer

	// Start to init and run loopRead
	Start(h Handler, dev Device, mtu int) (err error)
}
