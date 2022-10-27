//
// weproxy@foxmail.com 2022/10/20
//

package kc

import (
	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/stack/netstk"

	"weproxy/acc/app/acc/internal/proto"
)

const TAG = "[kc]"

func init() {
	proto.Register([]string{"kc", "kcp"}, New)
}

// New ...
func New(serv string) (proto.Handler, error) {
	return &Handler{}, nil
}

////////////////////////////////////////////////////////////////////////////////

// Handler implements proto.Handler
type Handler struct {
}

// Close ...
func (m *Handler) Close() error {
	return nil
}

// Handle TCP ...
func (m *Handler) Handle(c netstk.Conn, head []byte) {
	srcAddr, dstAddr := c.LocalAddr(), c.RemoteAddr()

	logx.D("%s Handle() %v->%v", TAG, srcAddr, dstAddr)
}

// HandlePacket ...
func (m *Handler) HandlePacket(pc netstk.PacketConn, head []byte) {
	srcAddr, dstAddr := pc.LocalAddr(), pc.RemoteAddr()

	logx.D("%s HandlePacket() %v->%v", TAG, srcAddr, dstAddr)
}
