//
// weproxy@foxmail.com 2022/10/20
//

package gvisor

import (
	"fmt"
	"log"
	"net"
	"runtime"
	"sync"
	"time"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/stack/netstk"

	"golang.org/x/time/rate"
	"gvisor.dev/gvisor/pkg/tcpip"
	"gvisor.dev/gvisor/pkg/tcpip/adapters/gonet"
	"gvisor.dev/gvisor/pkg/tcpip/buffer"
	"gvisor.dev/gvisor/pkg/tcpip/header"
	"gvisor.dev/gvisor/pkg/tcpip/link/channel"
	"gvisor.dev/gvisor/pkg/tcpip/network/ipv4"
	"gvisor.dev/gvisor/pkg/tcpip/network/ipv6"
	"gvisor.dev/gvisor/pkg/tcpip/stack"
	"gvisor.dev/gvisor/pkg/tcpip/transport/icmp"
	"gvisor.dev/gvisor/pkg/tcpip/transport/tcp"
	"gvisor.dev/gvisor/pkg/tcpip/transport/udp"
	"gvisor.dev/gvisor/pkg/waiter"
)

////////////////////////////////////////////////////////////////////////////////

// NewStack ...
func NewStack() (netstk.Stack, error) {
	return &Stack{udps: newUDPMap()}, nil
}

////////////////////////////////////////////////////////////////////////////////

// Stack implements netstk.Stack
type Stack struct {
	h    netstk.Handler
	dev  netstk.Device
	stk  *stack.Stack // gvisor
	udps *udpMap
}

// Start implements netstk.Stack
func (m *Stack) Start(h netstk.Handler, dev netstk.Device, mtu int) (err error) {
	m.h, m.dev = h, dev
	return m.initGvisor(dev, mtu)
}

func (m *Stack) initGvisor(dev netstk.Device, mtu int) (err error) {
	m.stk = stack.New(stack.Options{
		NetworkProtocols: []stack.NetworkProtocolFactory{
			ipv4.NewProtocol,
			ipv6.NewProtocol,
		},
		TransportProtocols: []stack.TransportProtocolFactory{
			tcp.NewProtocol,
			udp.NewProtocol,
			icmp.NewProtocol4,
			icmp.NewProtocol6,
		},
		HandleLocal: false,
	})

	// set NICID to 1
	const NICID = tcpip.NICID(1)

	// WithDefaultTTL sets the default TTL used by stack.
	{
		opt := tcpip.DefaultTTLOption(64)
		if tcperr := m.stk.SetNetworkProtocolOption(ipv4.ProtocolNumber, &opt); tcperr != nil {
			err = fmt.Errorf("set ipv4 default TTL: %s", tcperr)
			return
		}
		if tcperr := m.stk.SetNetworkProtocolOption(ipv6.ProtocolNumber, &opt); tcperr != nil {
			err = fmt.Errorf("set ipv6 default TTL: %s", tcperr)
			return
		}
	}

	// set forwarding
	if tcperr := m.stk.SetForwardingDefaultAndAllNICs(ipv4.ProtocolNumber, true); tcperr != nil {
		err = fmt.Errorf("set ipv4 forwarding error: %s", tcperr)
		return
	}
	if tcperr := m.stk.SetForwardingDefaultAndAllNICs(ipv6.ProtocolNumber, true); tcperr != nil {
		err = fmt.Errorf("set ipv6 forwarding error: %s", tcperr)
		return
	}

	// WithICMPBurst sets the number of ICMP messages that can be sent
	// in a single burst.
	m.stk.SetICMPBurst(50)

	// WithICMPLimit sets the maximum number of ICMP messages permitted
	// by rate limiter.
	m.stk.SetICMPLimit(rate.Limit(1000))

	// WithTCPBufferSizeRange sets the receive and send buffer size range for TCP.
	{
		rcvOpt := tcpip.TCPReceiveBufferSizeRangeOption{Min: stack.MinBufferSize, Default: stack.DefaultBufferSize, Max: stack.DefaultMaxBufferSize}
		if tcperr := m.stk.SetTransportProtocolOption(tcp.ProtocolNumber, &rcvOpt); tcperr != nil {
			err = fmt.Errorf("set TCP receive buffer size range: %s", tcperr)
			return
		}
		sndOpt := tcpip.TCPSendBufferSizeRangeOption{Min: stack.MinBufferSize, Default: stack.DefaultBufferSize, Max: stack.DefaultMaxBufferSize}
		if tcperr := m.stk.SetTransportProtocolOption(tcp.ProtocolNumber, &sndOpt); tcperr != nil {
			err = fmt.Errorf("set TCP send buffer size range: %s", tcperr)
			return
		}
	}

	// WithTCPCongestionControl sets the current congestion control algorithm.
	{
		opt := tcpip.CongestionControlOption("reno")
		if tcperr := m.stk.SetTransportProtocolOption(tcp.ProtocolNumber, &opt); tcperr != nil {
			err = fmt.Errorf("set TCP congestion control algorithm: %s", tcperr)
			return
		}
	}

	// WithTCPModerateReceiveBuffer sets receive buffer moderation for TCP.
	{
		opt := tcpip.TCPDelayEnabled(false)
		if tcperr := m.stk.SetTransportProtocolOption(tcp.ProtocolNumber, &opt); tcperr != nil {
			err = fmt.Errorf("set TCP delay: %s", err)
			return
		}
	}

	// WithTCPModerateReceiveBuffer sets receive buffer moderation for TCP.
	{
		opt := tcpip.TCPModerateReceiveBufferOption(true)
		if tcperr := m.stk.SetTransportProtocolOption(tcp.ProtocolNumber, &opt); tcperr != nil {
			err = fmt.Errorf("set TCP moderate receive buffer: %s", tcperr)
			return
		}
	}

	// WithTCPSACKEnabled sets the SACK option for TCP.
	{
		opt := tcpip.TCPSACKEnabled(true)
		if tcperr := m.stk.SetTransportProtocolOption(tcp.ProtocolNumber, &opt); tcperr != nil {
			err = fmt.Errorf("set TCP SACK: %s", tcperr)
			return
		}
	}

	mustSubnet := func(s string) tcpip.Subnet {
		_, ipNet, err := net.ParseCIDR(s)
		if err != nil {
			log.Panic(fmt.Errorf("unable to ParseCIDR(%s): %w", s, err))
		}

		subnet, err := tcpip.NewSubnet(tcpip.Address(ipNet.IP), tcpip.AddressMask(ipNet.Mask))
		if err != nil {
			log.Panic(fmt.Errorf("unable to NewSubnet(%s): %w", ipNet, err))
		}
		return subnet
	}

	// Add default route table for IPv4 and IPv6
	// This will handle all incoming ICMP packets.
	m.stk.SetRouteTable([]tcpip.Route{
		{
			// Destination: header.IPv4EmptySubnet,
			Destination: mustSubnet("0.0.0.0/0"),
			NIC:         NICID,
		},
		{
			// Destination: header.IPv6EmptySubnet,
			Destination: mustSubnet("::/0"),
			NIC:         NICID,
		},
	})

	// Important: We must initiate transport protocol handlers
	// before creating NIC, otherwise NIC would dispatch packets
	// to stack and cause race condition.
	m.stk.SetTransportProtocolHandler(tcp.ProtocolNumber, tcp.NewForwarder(m.stk, 16<<10, 1<<15, m.HandleStream).HandlePacket)
	m.stk.SetTransportProtocolHandler(udp.ProtocolNumber, m.HandlePacket)

	// WithCreatingNIC creates NIC for stack.
	if tcperr := m.stk.CreateNIC(NICID, NewEndpoint(dev, mtu)); tcperr != nil {
		err = fmt.Errorf("fail to create NIC in stack: %s", tcperr)
		return
	}

	// WithPromiscuousMode sets promiscuous mode in the given NIC.
	// In past we did m.AddAddressRange to assign 0.0.0.0/0 onto
	// the interface. We need that to be able to terminate all the
	// incoming connections - to any ip. AddressRange API has been
	// removed and the suggested workaround is to use Promiscuous
	// mode. https://gvisor.dev/gvisor/issues/3876
	//
	// Ref: https://github.com/majek/slirpnetstack/blob/master/stack.go
	if tcperr := m.stk.SetPromiscuousMode(NICID, true); tcperr != nil {
		err = fmt.Errorf("set promiscuous mode: %s", tcperr)
		return
	}

	// WithSpoofing sets address spoofing in the given NIC, allowing
	// endpoints to bind to any address in the NIC.
	// Enable spoofing if a stack may send packets from unowned addresses.
	// This change required changes to some netgophers since previously,
	// promiscuous mode was enough to let the netstack respond to all
	// incoming packets regardless of the packet's destination address. Now
	// that a stack.Route is not held for each incoming packet, finding a route
	// may fail with local addresses we don't own but accepted packets for
	// while in promiscuous mode. Since we also want to be able to send from
	// any address (in response the received promiscuous mode packets), we need
	// to enable spoofing.
	//
	// Ref: https://gvisor.dev/gvisor/commit/8c0701462a84ff77e602f1626aec49479c308127
	if tcperr := m.stk.SetSpoofing(NICID, true); tcperr != nil {
		err = fmt.Errorf("set spoofing: %s", tcperr)
		return
	}

	return nil
}

// HandleStream is to handle incoming TCP connections
func (m *Stack) HandleStream(r *tcp.ForwarderRequest) {
	id := r.ID()
	wq := waiter.Queue{}
	ep, tcperr := r.CreateEndpoint(&wq)
	if tcperr != nil {
		logx.E("tcp %v:%v <---> %v:%v create endpoint error: %v",
			net.IP(id.RemoteAddress),
			int(id.RemotePort),
			net.IP(id.LocalAddress),
			int(id.LocalPort),
			tcperr,
		)
		// prevent potential half-open TCP connection leak.
		r.Complete(true)
		return
	}
	r.Complete(false)

	// set keepalive
	if err := func(ep tcpip.Endpoint) error {
		ep.SocketOptions().SetKeepAlive(true)
		idleOpt := tcpip.KeepaliveIdleOption(60 * time.Second)
		if tcperr := ep.SetSockOpt(&idleOpt); tcperr != nil {
			return fmt.Errorf("set keepalive idle: %s", tcperr)
		}
		intervalOpt := tcpip.KeepaliveIntervalOption(30 * time.Second)
		if tcperr := ep.SetSockOpt(&intervalOpt); tcperr != nil {
			return fmt.Errorf("set keepalive interval: %s", tcperr)
		}
		return nil
	}(ep); err != nil {
		logx.E("tcp %v:%v <---> %v:%v create endpoint error: %v",
			net.IP(id.RemoteAddress),
			int(id.RemotePort),
			net.IP(id.LocalAddress),
			int(id.LocalPort),
			err,
		)
		return
	}

	conn := NewConn(gonet.NewTCPConn(&wq, ep))
	addr := &net.TCPAddr{IP: net.IP(id.LocalAddress), Port: int(id.LocalPort)}

	go m.h.Handle(conn, addr)
}

// HandlePacket is to handle UDP connections
func (m *Stack) HandlePacket(ep stack.TransportEndpointID, pkt *stack.PacketBuffer) bool {
	// Ref: gVisor pkg/tcpip/transport/udp/endpoint.go HandlePacket
	udpHdr := header.UDP(pkt.TransportHeader().View())
	if int(udpHdr.Length()) > pkt.Data().Size()+header.UDPMinimumSize {
		logx.E("udp %v:%v <---> %v:%v malformed packet",
			net.IP(ep.RemoteAddress),
			int(ep.RemotePort),
			net.IP(ep.LocalAddress),
			int(ep.LocalPort),
		)
		m.stk.Stats().UDP.MalformedPacketsReceived.Increment()
		return true
	}

	if !verifyChecksum(udpHdr, pkt) {
		logx.E("udp %v:%v <---> %v:%v checksum error",
			net.IP(ep.RemoteAddress),
			int(ep.RemotePort),
			net.IP(ep.LocalAddress),
			int(ep.LocalPort),
		)
		m.stk.Stats().UDP.ChecksumErrors.Increment()
		return true
	}

	m.stk.Stats().UDP.PacketsReceived.Increment()

	key := int(ep.RemotePort)
	if conn, ok := m.udps.Get(key); ok {
		vv := pkt.Data().ExtractVV()
		addr := &net.UDPAddr{IP: net.IP(ep.LocalAddress), Port: int(ep.LocalPort)}
		conn.HandlePacket(vv.ToView(), addr)
		return true
	}

	conn := NewPacketConn(key, ep, pkt, m)
	m.udps.Add(key, conn)
	vv := pkt.Data().ExtractVV()
	conn.HandlePacket(vv.ToView(), conn.LocalAddr().(*net.UDPAddr))

	if m.h == nil {
		panic("m.h == nil")
	}

	if conn.LocalAddr() == nil {
		panic("con.LocalAddr() == nil")
	}

	go m.h.HandlePacket(conn)

	return true
}

// Close is to close the stack
func (m *Stack) Close() error {
	m.udps.Clear()
	m.stk.Close()
	return nil
}

// PacketConn Table
type udpMap struct {
	mu    sync.RWMutex
	conns map[int]*PacketConn
}

// newUDPMap ...
func newUDPMap() *udpMap {
	return &udpMap{
		conns: make(map[int]*PacketConn),
	}
}

// Get is to get *PacketConn
func (m *udpMap) Get(k int) (*PacketConn, bool) {
	m.mu.RLock()
	conn, ok := m.conns[k]
	m.mu.RUnlock()
	return conn, ok
}

// Add is to add *PacketConn
func (m *udpMap) Add(k int, conn *PacketConn) {
	m.mu.Lock()
	m.conns[k] = conn
	m.mu.Unlock()
}

// Del is to delete *PacketConn
func (m *udpMap) Del(k int) {
	m.mu.Lock()
	delete(m.conns, k)
	m.mu.Unlock()
}

// Clear ..
func (m *udpMap) Clear() {
	m.mu.Lock()
	for _, conn := range m.conns {
		conn.Close()
	}
	m.conns = make(map[int]*PacketConn)
	m.mu.Unlock()
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Endpoint ...
type Endpoint struct {
	*channel.Endpoint
	dev netstk.Device
	mu  sync.Mutex
	mtu int
	buf []byte
}

// NewEndpoint ...
func NewEndpoint(dev netstk.Device, mtu int) stack.LinkEndpoint {
	ep := &Endpoint{
		Endpoint: channel.New(512, uint32(mtu), ""),
		dev:      dev,
		mtu:      mtu,
		buf:      make([]byte, mtu),
	}
	ep.Endpoint.AddNotify(ep)
	return ep
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// loopRead ...
func loopRead(r netstk.Device, size, offset int, ep *channel.Endpoint) {
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

	for {
		buf := make([]byte, size+offset)
		nr, err := r.Read(buf, offset)
		if err != nil {
			break
		}
		if nr == 0 {
			continue
		}
		dat := buf[offset : offset+nr]

		pkt := stack.NewPacketBuffer(stack.PacketBufferOptions{
			ReserveHeaderBytes: 0,
			Data:               buffer.View(dat).ToVectorisedView(),
		})

		switch ver := header.IPVersion(dat); ver {
		case header.IPv4Version:
			ep.InjectInbound(header.IPv4ProtocolNumber, pkt)
		case header.IPv6Version:
			ep.InjectInbound(header.IPv6ProtocolNumber, pkt)
		}
	}
}
