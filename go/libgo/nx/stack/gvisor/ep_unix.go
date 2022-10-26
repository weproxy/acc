//
// weproxy@foxmail.com 2022/10/20
//

//go:build linux || darwin
// +build linux darwin

package gvisor

import (
	"gvisor.dev/gvisor/pkg/tcpip/stack"
)

const _offset = 4

// Attach is to attach device to stack
func (m *Endpoint) Attach(dispatcher stack.NetworkDispatcher) {
	m.Endpoint.Attach(dispatcher)

	go loopRead(m.dev, _offset+m.mtu, _offset, m.Endpoint)
}

// WriteNotify is to write packets back to system
func (m *Endpoint) WriteNotify() {
	pkt := m.Endpoint.Read()
	if pkt == nil {
		return
	}

	m.mu.Lock()

	buf := append(m.buf[:_offset], pkt.NetworkHeader().View()...)
	buf = append(buf, pkt.TransportHeader().View()...)
	vv := pkt.Data().ExtractVV()
	buf = append(buf, vv.ToView()...)

	m.dev.Write(buf, _offset)

	m.mu.Unlock()
}
