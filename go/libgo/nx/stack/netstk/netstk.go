//
// weproxy@foxmail.com 2022/10/20
//

package netstk

import (
	"io"
	"net"
	"time"
)

// baseConn ...
type baseConn interface {
	io.Closer

	ID() uint64
	LocalAddr() net.Addr
	RemoteAddr() net.Addr
	SetDeadline(t time.Time) error
	SetReadDeadline(t time.Time) error
	SetWriteDeadline(t time.Time) error
}

// Conn ...
type Conn interface {
	baseConn
	io.ReadWriter
	CloseRead() error
	CloseWrite() error
}

// PacketConn ...
type PacketConn interface {
	baseConn

	// ReadTo readFrom device to dstAddr
	ReadTo(p []byte) (n int, dstAddr net.Addr, err error)

	// WriteFrom writeTo device from srcAddr
	WriteFrom(p []byte, srcAddr net.Addr) (int, error)
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
	Handle(Conn, net.Addr)

	// HandlePacket UDP ...
	HandlePacket(PacketConn)
}

// Stack ...
type Stack interface {
	io.Closer

	// Start to init and run loopRead
	Start(h Handler, dev Device, mtu int) (err error)
}
