//
// weproxy@foxmail.com 2022/10/20
//

package proto

import (
	"net"

	"weproxy/acc/libgo/nx/stack"
	"weproxy/acc/libgo/nx/stack/netstk"
)

var _stk netstk.Stack

// Init ...
func Init() error {
	stk, err := stack.NewStack()
	if err != nil {
		return err
	}

	_stk = stk

	var dev netstk.Device

	stk.Start(&Handler{}, dev, 1500)

	return nil
}

// Deinit ...
func Deinit() error {
	if _stk != nil {
		_stk.Close()
		_stk = nil
	}
	return nil
}

// Handler implements netstk.Handler
type Handler struct {
}

// Close ...
func (m *Handler) Close() error {
	return nil
}

// Handle TCP ...
func (m *Handler) Handle(netstk.Conn, net.Addr) {

}

// HandlePacket ...
func (m *Handler) HandlePacket(netstk.PacketConn) {

}
