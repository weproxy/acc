//
// weproxy@foxmail.com 2022/10/20
//

package proto

import (
	"net"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/device"
	_ "weproxy/acc/libgo/nx/device/mod"
	"weproxy/acc/libgo/nx/stack"
	"weproxy/acc/libgo/nx/stack/netstk"
)

const TAG = "proto"

var _dev netstk.Device
var _stk netstk.Stack

// Init ...
func Init() error {
	stk, err := stack.NewStack()
	if err != nil {
		return err
	}

	dev, err := device.NewDevice(device.TypeTUN, nil)
	if err != nil {
		return err
	}

	err = stk.Start(&Handler{}, dev, 1500)
	if err != nil {
		dev.Close()
		return err
	}

	_dev, _stk = dev, stk

	return nil
}

// Deinit ...
func Deinit() error {
	if _dev != nil {
		_dev.Close()
		_dev = nil
	}
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
func (m *Handler) Handle(c netstk.Conn, addr net.Addr) {
	logx.D("%s Handle() addr=%v", addr)
}

// HandlePacket ...
func (m *Handler) HandlePacket(pc netstk.PacketConn) {
	logx.D("%s HandlePacket() addr=%v", pc.LocalAddr())
}
