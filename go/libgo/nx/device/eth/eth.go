//
// weproxy@foxmail.com 2022/10/20
//

package eth

import (
	"context"
	"errors"
	"io"
	"net"
	"sync"

	"github.com/google/gopacket"
	"github.com/google/gopacket/layers"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/device"
	"weproxy/acc/libgo/nx/util"
)

// Inner ...
type Inner interface {
	io.Closer
	Type() string
	Open(ifc *net.Interface, cidr net.IPNet) error
	ReadPacketData() (data []byte, ci gopacket.CaptureInfo, err error)
	WritePacketData(data []byte) (err error)
}

// ethDevice implements netstk.Device
type ethDevice struct {
	p struct {
		*io.PipeReader
		*io.PipeWriter
	}
	ifc struct {
		index int
		name  string
		mac   net.HardwareAddr
	}
	inner    Inner
	macMap   sync.Map // net.IP ->  net.HardwareAddr
	cancelFn context.CancelFunc
	closedCh chan struct{}
	ipnet    net.IPNet
}

// NewInnerFn ....
type NewInnerFn func(ifc *net.Interface, cidr net.IPNet) (Inner, error)

// New ...
func New(ifname, cidr string, newInnerFn NewInnerFn) (dev *ethDevice, err error) {
	m := &ethDevice{
		closedCh: make(chan struct{}),
	}

	m.p.PipeReader, m.p.PipeWriter = io.Pipe()

	defer func() {
		if err != nil {
			close(m.closedCh)
			m.Close()
		}
	}()

	ip, ipnet, err := net.ParseCIDR(cidr)
	if err != nil {
		logx.E("[eth] ParseCIDR(%s) fail: %v", cidr, err)
		return
	}

	ifc, err := util.GetDefaultInterface(ifname)
	if err != nil {
		logx.E("[eth] get interface fail: %v", err)
		return
	}
	m.ifc.index, m.ifc.name, m.ifc.mac = ifc.Index, ifc.Name, ifc.HardwareAddr

	logx.I("[eth] getDefaultInterface(): ifIdx=%v, ifName=%v", ifc.Index, ifc.Name)

	// ipnet = 10.6.6.0/24
	//  -> 10.6.6.1/24 to util.AddAdapterIP()/DelAdapterIP()
	m.ipnet = net.IPNet{IP: ip, Mask: ipnet.Mask}

	if err = util.AddAdapterIP(ifc.Index, m.ipnet); err != nil {
		logx.E("[eth] AddAdapterIP(%s, %v) %v", ifc.Name, m.ipnet, err)
		return
	}
	logx.I("[eth] AddAdapterIP(%s, %v)", ifc.Name, m.ipnet.String())

	defer func() {
		if err != nil {
			util.DelAdapterIP(m.ifc.index, m.ipnet)
			logx.E("[eth] Open(%v, %v) %v", ifname, cidr, err)
			logx.I("[eth] DelAdapterIP(%s, %v)", ifc.Name, m.ipnet.String())
		}
	}()

	m.inner, err = newInnerFn(ifc, m.ipnet)
	if err != nil {
		logx.E("[eth] newInnerFn() %v", err)
		return
	}

	ctx, cancelFn := context.WithCancel(context.Background())
	m.cancelFn = cancelFn

	// linux RAW use iptables
	if m.inner.Type() != device.TypeRAW {
		go m.run(ctx) // other pcap etc ...
	}

	return m, nil
}

// Close ...
func (m *ethDevice) Close() (err error) {
	select {
	case <-m.closedCh:
		return nil
	default:
	}

	if m.cancelFn != nil {
		m.cancelFn()
	}

	if m.inner != nil {
		m.inner.Close()
	}

	if m.p.PipeReader != nil {
		m.p.PipeReader.Close()
		m.p.PipeWriter.Close()
	}

	if m.ifc.index >= 0 && len(m.ipnet.IP) > 0 {
		util.DelAdapterIP(m.ifc.index, m.ipnet)
		logx.I("[eth] DelAdapterIP(%s, %v)", m.ifc.name, m.ipnet.String())
	}

	if m.inner != nil && m.inner.Type() != "raw" {
		<-m.closedCh
	}

	return
}

// Type impl netstk.Device
func (m *ethDevice) Type() string {
	if m.inner != nil {
		return m.inner.Type()
	}
	return "eth"
}

// ReadPacketData impl gopacket.PacketDataSource
func (m *ethDevice) ReadPacketData() (data []byte, ci gopacket.CaptureInfo, err error) {
	if m.inner != nil {
		return m.inner.ReadPacketData()
	}
	err = io.ErrClosedPipe
	return
}

// WritePacketData ...
func (m *ethDevice) WritePacketData(data []byte) (err error) {
	if m.inner != nil {
		return m.inner.WritePacketData(data)
	}
	return io.ErrClosedPipe
}

// run ...
func (m *ethDevice) run(ctx context.Context) {
	defer func() {
		if e := recover(); e != nil {
			logx.E("%v", e)
		}

		close(m.closedCh)
	}()

	ps := gopacket.NewPacketSource(m, layers.LinkTypeEthernet)

	for {
		select {
		case <-ctx.Done():
			return
		case p := <-ps.Packets():
			if false {
				if arpLayer := p.Layer(layers.LayerTypeARP); arpLayer != nil {
					m.handleARPLayer(p, arpLayer)
				} else if ip4Layer := p.Layer(layers.LayerTypeIPv4); ip4Layer != nil {
					m.handleIP4Layer(p, ip4Layer)
				}
			} else {
				for _, lay := range p.Layers() {
					switch lay := lay.(type) {
					case *layers.ARP:
						m.handleARPLayer(p, lay)
					case *layers.IPv4:
						m.handleIP4Layer(p, lay)
					}
				}
			}
		}
	}
}

// Read ...
func (m *ethDevice) Read(p []byte, offset int) (n int, err error) {
	// logx.D("[eth] Read(p, %v)\n", offset)
	if m.p.PipeReader != nil {
		return m.p.Read(p[offset:])
	}
	return 0, io.EOF
}

// Write ...
func (m *ethDevice) Write(p []byte, offset int) (n int, err error) {
	p = p[offset:]
	// logx.D("[eth] Write(len(p)=%v)\n", len(p))

	if m.inner == nil {
		return 0, io.ErrClosedPipe
	}

	packet := gopacket.NewPacket(p, layers.LayerTypeIPv4, gopacket.Default)

	ip4Lay := packet.Layer(layers.LayerTypeIPv4)
	if ip4Lay == nil {
		return len(p), nil
	}
	ip4Pkt := ip4Lay.(*layers.IPv4)

	var dstMac net.HardwareAddr
	v, ok := m.macMap.Load(ip4Pkt.DstIP.String())
	if ok {
		dstMac, ok = v.(net.HardwareAddr)
	}
	if !ok {
		logx.W("[eth] !!!! not found MAC of %v", ip4Pkt.DstIP.String())
		return len(p), nil
	}

	ethLay := &layers.Ethernet{
		EthernetType: layers.EthernetTypeIPv4,
		SrcMAC:       m.ifc.mac,
		DstMAC:       dstMac,
	}

	// fragment
	err = util.ForwardEthPacketFragment(ethLay, ip4Pkt, 1500, func(data []byte) (err error) {
		err = m.WritePacketData(data)
		if err != nil {
			logx.W("[eth] WritePacketData(%v/%v, %dbytes), err=%v", ip4Pkt.DstIP, dstMac, len(data), err)
		}
		return
	})

	return len(p), err
}

// handleIP4Layer ...
func (m *ethDevice) handleIP4Layer(p gopacket.Packet, lay gopacket.Layer) {
	ip4Pkt, ok := lay.(*layers.IPv4)
	if !ok {
		// println("!IPv4")
		return
	}

	// only 10.6.6.x but not 10.6.6.1
	srcIP, dstIP := ip4Pkt.SrcIP, ip4Pkt.DstIP
	if srcIP[3] == 1 || !m.ipnet.Contains(srcIP) {
		// logx.W("[eth] IPv4 ipnet(%v).!Contains(%v)", m.ipnet.String(), srcIP)
		return
	}
	// logx.D("[eth] IPv4 %v->%v", srcIP, dstIP)
	if _, ok := m.macMap.Load(srcIP.String()); !ok {
		if ethLayer := p.Layer(layers.LayerTypeEthernet); ethLayer != nil {
			if ethPkt, ok := ethLayer.(*layers.Ethernet); ok {
				m.macMap.Store(srcIP.String(), ethPkt.SrcMAC)
				logx.I("!!!! add %v->%v", srcIP.String(), ethPkt.SrcMAC)
			}
		}
	}
	_ = dstIP

	d := append([]byte{}, ip4Pkt.Contents...)
	d = append(d, ip4Pkt.Payload...)
	m.p.Write(d)
}

// OnArpRequest...
var OnArpRequestCallback func(senderIP, targetIP net.IP)

// handleARPLayer ...
func (m *ethDevice) handleARPLayer(p gopacket.Packet, lay gopacket.Layer) {
	arpPkt, ok := lay.(*layers.ARP)
	if !ok {
		return
	}

	if arpPkt.Operation != layers.ARPRequest {
		return
	}

	senderIP := net.IP(arpPkt.SourceProtAddress)
	targetIP := net.IP(arpPkt.DstProtAddress)

	if m.ipnet.Contains(senderIP) {
		if _, ok := m.macMap.Load(senderIP.String()); !ok {
			mac := net.HardwareAddr(arpPkt.SourceHwAddress)
			m.macMap.Store(senderIP.String(), mac)
			logx.I("!!!! add %v->%v", senderIP.String(), mac)
		}
	}

	// logx.D("[eth] ARP request %v->%v", senderIP, targetIP)

	if _, err := findMatchedMac(senderIP); err == nil {
		// it's me
		return
	}
	mac, err := findMatchedMac(targetIP)
	if err != nil {
		return
	}

	if OnArpRequestCallback != nil {
		OnArpRequestCallback(senderIP, targetIP)
	}

	logx.D("[arp] %s ask: Who has %s?", senderIP, targetIP)
	if false {
		return
	}

	ethPkt := makeEthArpPacket(layers.ARPReply, targetIP, senderIP, mac, arpPkt.SourceHwAddress)

	if err = m.WritePacketData(ethPkt); err == nil {
		logx.I("[arp] We reply: %s is at %s", targetIP, mac)
	} else {
		logx.W("[arp] We reply: %s is at %s, err=%v", targetIP, mac, err)
	}
}

// findMatchedMac ...
func findMatchedMac(ipfind net.IP) (mac net.HardwareAddr, err error) {
	ifaces, err := net.Interfaces()
	if err != nil {
		return
	}

	for _, ifce := range ifaces {
		if ifce.Flags&net.FlagUp == 0 || ifce.Flags&net.FlagLoopback != 0 {
			continue
		}

		addrs, err := ifce.Addrs()
		if err == nil {
			for _, ifa := range addrs {
				switch ifa := ifa.(type) {
				case *net.IPAddr:
					if ifa.IP.Equal(ipfind) {
						return ifce.HardwareAddr, nil
					}
				case *net.IPNet:
					if ifa.IP.Equal(ipfind) {
						return ifce.HardwareAddr, nil
					}
				}
			}
		}
	}

	return mac, errors.New("not found")
}

// makeEthArpPacket ...
func makeEthArpPacket(operation uint16, srcIP, dstIP net.IP, srcMAC, dstMAC net.HardwareAddr) []byte {
	eth := &layers.Ethernet{
		SrcMAC:       srcMAC,
		DstMAC:       dstMAC,
		EthernetType: layers.EthernetTypeARP,
	}

	arp := &layers.ARP{
		AddrType:          layers.LinkTypeEthernet,
		Protocol:          layers.EthernetTypeIPv4,
		HwAddressSize:     uint8(6),
		ProtAddressSize:   uint8(4),
		Operation:         operation,
		SourceHwAddress:   srcMAC,
		SourceProtAddress: srcIP,
		DstHwAddress:      dstMAC,
		DstProtAddress:    dstIP,
	}

	buf := gopacket.NewSerializeBuffer()
	var opt gopacket.SerializeOptions
	gopacket.SerializeLayers(buf, opt, eth, arp)

	return buf.Bytes()
}
