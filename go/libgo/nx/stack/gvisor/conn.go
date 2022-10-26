//
// weproxy@foxmail.com 2022/10/20
//

package gvisor

import (
	"errors"
	"io"
	"net"
	"sync"
	"time"
	"unsafe"
	_ "unsafe" // for go:linkname

	"weproxy/acc/libgo/nx"

	"gvisor.dev/gvisor/pkg/tcpip"
	"gvisor.dev/gvisor/pkg/tcpip/adapters/gonet"
	"gvisor.dev/gvisor/pkg/tcpip/buffer"
	"gvisor.dev/gvisor/pkg/tcpip/header"
	"gvisor.dev/gvisor/pkg/tcpip/stack"
)

////////////////////////////////////////////////////////////////////////////////

// Conn implements netstk.Conn
type Conn struct {
	*gonet.TCPConn
	id uint64
}

// ID ...
func (m *Conn) ID() uint64 {
	return m.id
}

// NewConn ...
func NewConn(c *gonet.TCPConn) *Conn {
	return &Conn{TCPConn: c, id: nx.NewID()}
}

////////////////////////////////////////////////////////////////////////////////

// Packet ...
type Packet struct {
	Addr *net.UDPAddr
	Byte []byte
}

// PacketConn implements netstk.PacketConn
type PacketConn struct {
	deadlineTimer
	id   uint64
	key  int
	stk  *Stack
	info struct {
		src  tcpip.Address
		nic  tcpip.NICID
		prot tcpip.NetworkProtocolNumber
		ep   stack.TransportEndpointID
	}
	stream chan Packet
	closed chan struct{}
}

// NewPacketConn ...
func NewPacketConn(key int, ep stack.TransportEndpointID, pkt *stack.PacketBuffer, stk *Stack) *PacketConn {
	m := &PacketConn{id: nx.NewID(), key: key,
		stk:    stk,
		stream: make(chan Packet, 16),
		closed: make(chan struct{}),
	}
	hdr := pkt.Network()
	m.info.src = hdr.SourceAddress()
	m.info.nic = pkt.NICID
	m.info.prot = pkt.NetworkProtocolNumber
	m.info.ep = ep
	m.deadlineTimer.init()
	return m
}

// ID ...
func (m *PacketConn) ID() uint64 {
	return m.id
}

// Close ...
func (m *PacketConn) Close() error {
	select {
	case <-m.closed:
		return nil
	default:
		close(m.closed)
	}
	m.stk.udps.Del(m.key)
	return nil
}

// LocalAddr ...
func (m *PacketConn) LocalAddr() net.Addr {
	return &net.UDPAddr{IP: net.IP(m.info.ep.LocalAddress), Port: int(m.info.ep.LocalPort)}
}

// RemoteAddr ...
func (m *PacketConn) RemoteAddr() net.Addr {
	return &net.UDPAddr{IP: net.IP(m.info.ep.RemoteAddress), Port: int(m.info.ep.RemotePort)}
}

// ReadTo readFrom device to dstAddr
func (m *PacketConn) ReadTo(p []byte) (n int, dstAddr net.Addr, err error) {
	deadline := m.readCancel()
	select {
	case <-deadline:
		err = (*timeoutError)(nil)
	case <-m.closed:
		err = io.EOF
	case pkt := <-m.stream:
		n = copy(p, pkt.Byte)
		dstAddr = pkt.Addr
	}
	return
}

// WriteFrom writeTo device from srcAddr
func (m *PacketConn) WriteFrom(p []byte, srcAddr net.Addr) (n int, err error) {
	v := buffer.View(p)
	if len(v) > header.UDPMaximumPacketSize {
		return 0, errors.New((&tcpip.ErrMessageTooLong{}).String())
	}

	src, ok := srcAddr.(*net.UDPAddr)
	if !ok {
		return 0, errors.New("gvisor.WriteFrom error: addr type error")
	}

	route, tcperr := m.stk.stk.FindRoute(m.info.nic, tcpip.Address(src.IP), m.info.src, m.info.prot, false)
	if tcperr != nil {
		return 0, errors.New(tcperr.String())
	}
	defer route.Release()

	n, tcperr = (&udpPacketInfo{
		route:         route,
		data:          v,
		localPort:     uint16(src.Port),
		remotePort:    m.info.ep.RemotePort,
		ttl:           0,    /* ttl */
		useDefaultTTL: true, /* useDefaultTTL */
		tos:           0,    /* tos */
		owner:         nil,  /* owner */
		noChecksum:    true,
	}).send()
	if tcperr != nil {
		return n, errors.New(tcperr.String())
	}

	return n, nil
}

// HandlePacket is to read packet to PacketConn
func (m *PacketConn) HandlePacket(b []byte, addr *net.UDPAddr) {
	select {
	case <-m.closed:
	case m.stream <- Packet{Addr: addr, Byte: b}:
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// verifyChecksum verifies the checksum unless RX checksum offload is enabled.
// On IPv4, UDP checksum is optional, and a zero value means the transmitter
// omitted the checksum generation (RFC768).
// On IPv6, UDP checksum is not optional (RFC2460 Section 8.1).
//
//go:linkname verifyChecksum gvisor.dev/gvisor/pkg/tcpip/transport/udp.verifyChecksum
func verifyChecksum(hdr header.UDP, pkt *stack.PacketBuffer) bool

////////////////////////////////////////////////////////////////////////////////////////////////////

// timeoutError is how the net package reports timeouts.
type timeoutError struct{}

func (e *timeoutError) Error() string   { return "i/o timeout" }
func (e *timeoutError) Timeout() bool   { return true }
func (e *timeoutError) Temporary() bool { return true }

////////////////////////////////////////////////////////////////////////////////////////////////////

// deadlineTimer ...
type deadlineTimer struct {
	// mu protects the fields below.
	mu sync.Mutex

	readTimer     *time.Timer
	readCancelCh  chan struct{}
	writeTimer    *time.Timer
	writeCancelCh chan struct{}
}

func (m *deadlineTimer) init() {
	m.readCancelCh = make(chan struct{})
	m.writeCancelCh = make(chan struct{})
}

func (m *deadlineTimer) readCancel() <-chan struct{} {
	m.mu.Lock()
	c := m.readCancelCh
	m.mu.Unlock()
	return c
}

func (m *deadlineTimer) writeCancel() <-chan struct{} {
	m.mu.Lock()
	c := m.writeCancelCh
	m.mu.Unlock()
	return c
}

// setDeadline contains the shared logic for setting a deadline.
//
// cancelCh and timer must be pointers to deadlineTimer.readCancelCh and
// deadlineTimer.readTimer or deadlineTimer.writeCancelCh and
// deadlineTimer.writeTimer.
//
// setDeadline must only be called while holding m.mu.
func (m *deadlineTimer) setDeadline(cancelCh *chan struct{}, timer **time.Timer, t time.Time) {
	if *timer != nil && !(*timer).Stop() {
		*cancelCh = make(chan struct{})
	}

	// Create a new channel if we already closed it due to setting an already
	// expired time. We won't race with the timer because we already handled
	// that above.
	select {
	case <-*cancelCh:
		*cancelCh = make(chan struct{})
	default:
	}

	// "A zero value for t means I/O operations will not time out."
	// - net.Conn.SetDeadline
	if t.IsZero() {
		return
	}

	timeout := time.Until(t)
	if timeout <= 0 {
		close(*cancelCh)
		return
	}

	// Timer.Stop returns whether or not the AfterFunc has started, but
	// does not indicate whether or not it has completed. Make a copy of
	// the cancel channel to prevent this code from racing with the next
	// call of setDeadline replacing *cancelCh.
	ch := *cancelCh
	*timer = time.AfterFunc(timeout, func() {
		close(ch)
	})
}

// SetReadDeadline implements net.Conn.SetReadDeadline and
// net.PacketConn.SetReadDeadline.
func (m *deadlineTimer) SetReadDeadline(t time.Time) error {
	m.mu.Lock()
	m.setDeadline(&m.readCancelCh, &m.readTimer, t)
	m.mu.Unlock()
	return nil
}

// SetWriteDeadline implements net.Conn.SetWriteDeadline and
// net.PacketConn.SetWriteDeadline.
func (m *deadlineTimer) SetWriteDeadline(t time.Time) error {
	m.mu.Lock()
	m.setDeadline(&m.writeCancelCh, &m.writeTimer, t)
	m.mu.Unlock()
	return nil
}

// SetDeadline implements net.Conn.SetDeadline and net.PacketConn.SetDeadline.
func (m *deadlineTimer) SetDeadline(t time.Time) error {
	m.mu.Lock()
	m.setDeadline(&m.readCancelCh, &m.readTimer, t)
	m.setDeadline(&m.writeCancelCh, &m.writeTimer, t)
	m.mu.Unlock()
	return nil
}

////////////////////////////////////////////////////////////////////////////////

// use unsafe package
var _ unsafe.Pointer = unsafe.Pointer(nil)

// udpPacketInfo contains all information required to send a UDP packet.
//
// This should be used as a value-only type, which exists in order to simplify
// return value syntax. It should not be exported or extended.
type udpPacketInfo struct {
	route         *stack.Route
	data          buffer.View
	localPort     uint16
	remotePort    uint16
	ttl           uint8
	useDefaultTTL bool
	tos           uint8
	owner         tcpip.PacketOwner
	noChecksum    bool
}

// send sends the given packet.
func (m *udpPacketInfo) send() (int, tcpip.Error) {
	const ProtocolNumber = header.UDPProtocolNumber

	vv := m.data.ToVectorisedView()
	pkt := stack.NewPacketBuffer(stack.PacketBufferOptions{
		ReserveHeaderBytes: header.UDPMinimumSize + int(m.route.MaxHeaderLength()),
		Data:               vv,
	})
	pkt.Owner = m.owner

	// Initialize the UDP header.
	udp := header.UDP(pkt.TransportHeader().Push(header.UDPMinimumSize))
	pkt.TransportProtocolNumber = ProtocolNumber

	length := uint16(pkt.Size())
	udp.Encode(&header.UDPFields{
		SrcPort: m.localPort,
		DstPort: m.remotePort,
		Length:  length,
	})

	// Set the checksum field unless TX checksum offload is enabled.
	// On IPv4, UDP checksum is optional, and a zero value indicates the
	// transmitter skipped the checksum generation (RFC768).
	// On IPv6, UDP checksum is not optional (RFC2460 Section 8.1).
	if m.route.RequiresTXTransportChecksum() &&
		(!m.noChecksum || m.route.NetProto() == header.IPv6ProtocolNumber) {
		xsum := m.route.PseudoHeaderChecksum(ProtocolNumber, length)
		for _, v := range vv.Views() {
			xsum = header.Checksum(v, xsum)
		}
		udp.SetChecksum(^udp.CalculateChecksum(xsum))
	}

	if m.useDefaultTTL {
		m.ttl = m.route.DefaultTTL()
	}
	if err := m.route.WritePacket(stack.NetworkHeaderParams{
		Protocol: ProtocolNumber,
		TTL:      m.ttl,
		TOS:      m.tos,
	}, pkt); err != nil {
		m.route.Stats().UDP.PacketSendErrors.Increment()
		return 0, err
	}

	// Track count of packets sent.
	m.route.Stats().UDP.PacketsSent.Increment()
	return len(m.data), nil
}
