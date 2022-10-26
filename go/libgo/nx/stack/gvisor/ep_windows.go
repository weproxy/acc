//
// weproxy@foxmail.com 2022/10/20
//

package gvisor

import (
	"io"
	"unsafe"

	"gvisor.dev/gvisor/pkg/tcpip/buffer"
	"gvisor.dev/gvisor/pkg/tcpip/header"
	"gvisor.dev/gvisor/pkg/tcpip/link/channel"
	"gvisor.dev/gvisor/pkg/tcpip/stack"
)

// Attach is to attach device to stack
func (m *Endpoint) Attach(dispatcher stack.NetworkDispatcher) {
	m.Endpoint.Attach(dispatcher)

	// WinDivert has no Reader
	wt, ok := m.dev.(io.WriterTo)
	if ok {
		go func(w io.Writer, wt io.WriterTo) {
			if _, err := wt.WriteTo(w); err != nil {
				return
			}
		}((*endpoint)(unsafe.Pointer(m.Endpoint)), wt)
		return
	}

	// WinTun
	go loopRead(m.dev, m.mtu+4, 4, m.Endpoint)
}

// WriteNotify is to write packets back to system
func (m *Endpoint) WriteNotify() {
	pkt := m.Endpoint.Read()
	if pkt == nil {
		return
	}

	m.mu.Lock()
	buf := append(m.buf[:0], pkt.NetworkHeader().View()...)
	buf = append(buf, pkt.TransportHeader().View()...)
	vv := pkt.Data().ExtractVV()
	buf = append(buf, vv.ToView()...)
	m.dev.Write(buf, 0)
	m.mu.Unlock()
}

// endpoint is for WinDivert
// write packets from WinDivert to gvisor netstack
type endpoint struct {
	Endpoint channel.Endpoint
}

// Write is to write packet to stack
func (m *endpoint) Write(b []byte) (int, error) {
	buf := append(make([]byte, 0, len(b)), b...)

	switch header.IPVersion(buf) {
	case header.IPv4Version:
		// WinDivert: need to calculate chekcsum
		pkt := header.IPv4(buf)
		pkt.SetChecksum(0)
		pkt.SetChecksum(^pkt.CalculateChecksum())
		switch ProtocolNumber := pkt.TransportProtocol(); ProtocolNumber {
		case header.UDPProtocolNumber:
			hdr := header.UDP(pkt.Payload())
			sum := header.PseudoHeaderChecksum(ProtocolNumber, pkt.DestinationAddress(), pkt.SourceAddress(), hdr.Length())
			sum = header.Checksum(hdr.Payload(), sum)
			hdr.SetChecksum(0)
			hdr.SetChecksum(^hdr.CalculateChecksum(sum))
		case header.TCPProtocolNumber:
			hdr := header.TCP(pkt.Payload())
			sum := header.PseudoHeaderChecksum(ProtocolNumber, pkt.DestinationAddress(), pkt.SourceAddress(), pkt.PayloadLength())
			sum = header.Checksum(hdr.Payload(), sum)
			hdr.SetChecksum(0)
			hdr.SetChecksum(^hdr.CalculateChecksum(sum))
		}
		m.Endpoint.InjectInbound(header.IPv4ProtocolNumber, stack.NewPacketBuffer(stack.PacketBufferOptions{
			ReserveHeaderBytes: 0,
			Data:               buffer.View(buf).ToVectorisedView(),
		}))
	case header.IPv6Version:
		// WinDivert: need to calculate chekcsum
		pkt := header.IPv6(buf)
		switch ProtocolNumber := pkt.TransportProtocol(); ProtocolNumber {
		case header.UDPProtocolNumber:
			hdr := header.UDP(pkt.Payload())
			sum := header.PseudoHeaderChecksum(ProtocolNumber, pkt.DestinationAddress(), pkt.SourceAddress(), hdr.Length())
			sum = header.Checksum(hdr.Payload(), sum)
			hdr.SetChecksum(0)
			hdr.SetChecksum(^hdr.CalculateChecksum(sum))
		case header.TCPProtocolNumber:
			hdr := header.TCP(pkt.Payload())
			sum := header.PseudoHeaderChecksum(ProtocolNumber, pkt.DestinationAddress(), pkt.SourceAddress(), pkt.PayloadLength())
			sum = header.Checksum(hdr.Payload(), sum)
			hdr.SetChecksum(0)
			hdr.SetChecksum(^hdr.CalculateChecksum(sum))
		}
		m.Endpoint.InjectInbound(header.IPv6ProtocolNumber, stack.NewPacketBuffer(stack.PacketBufferOptions{
			ReserveHeaderBytes: 0,
			Data:               buffer.View(buf).ToVectorisedView(),
		}))
	}
	return len(buf), nil
}
