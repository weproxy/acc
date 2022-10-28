//
// weproxy@foxmail.com 2022/10/20
//

package pcap

import (
	"errors"
	"fmt"
	"net"

	"github.com/google/gopacket"
	"github.com/google/gopacket/pcap"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/device"
	"weproxy/acc/libgo/nx/device/eth"
	"weproxy/acc/libgo/nx/stack/netstk"
)

// init ...
func init() {
	device.Register(device.TypePCAP, New)
}

// New ...
func New(cfg device.Conf) (netstk.Device, error) {
	ifname, cidr := cfg.Str("ifname"), cfg.Str("cidr")
	if len(ifname) == 0 || len(cidr) == 0 {
		return nil, errors.New("missing cfg ifname/cidr")
	}

	newInnerFn := func(ifc *net.Interface, cidr net.IPNet) (eth.Inner, error) {
		inner := &pcapDevice{}
		if err := inner.Open(ifc, cidr); err != nil {
			return nil, err
		}
		return inner, nil
	}

	return eth.New(ifname, cidr, newInnerFn)
}

////////////////////////////////////////////////////////////////////////////////

// pcapDevice implements eth.Inner
type pcapDevice struct {
	h *pcap.Handle
}

// Type ...
func (m *pcapDevice) Type() string {
	return device.TypePCAP
}

// Close implements io.Closer
func (m *pcapDevice) Close() error {
	if m.h != nil {
		m.h.Close()
	}
	return nil
}

// Open ...
func (m *pcapDevice) Open(ifc *net.Interface, cidr net.IPNet) (err error) {
	ip, ipnet, err := net.ParseCIDR(cidr.String())
	if err != nil {
		logx.E("[pcap] ParseCIDR(%v) %v", cidr, err)
		return
	}

	devName, err := m.getDevName(ifc)
	if err != nil {
		logx.E("[pcap] getDevName(%v) %v", ifc.Name, err)
		return
	}

	m.h, err = pcap.OpenLive(devName, 2048, true, pcap.BlockForever)
	if err != nil {
		logx.E("[pcap] pcap.OpenLive(%v) %v", devName, err)
		return
	}

	filter := fmt.Sprintf("arp or (ip and not src host %v and src net %s)", ip.String(), ipnet.String())
	if err = m.h.SetBPFFilter(filter); err != nil {
		logx.E("[pcap] pcap.SetBPFFilter(\"%v\") %v", filter, err)
		return
	}

	logx.P("[pcap] %v", filter)

	return
}

// ReadPacketData for gopacket interface
func (m *pcapDevice) ReadPacketData() (data []byte, ci gopacket.CaptureInfo, err error) {
	return m.h.ReadPacketData()
}

// WritePacketData ...
func (m *pcapDevice) WritePacketData(data []byte) (err error) {
	return m.h.WritePacketData(data)
}

// getDevName ...
func (m *pcapDevice) getDevName(ifc *net.Interface) (devName string, err error) {
	devices, err := pcap.FindAllDevs()
	if err != nil {
		return
	}

	ifAddrs, err := ifc.Addrs()
	if err != nil {
		return
	}

	matchIfAddrFn := func(devAddr *pcap.InterfaceAddress) bool {
		for _, ifa := range ifAddrs {
			switch ifa := ifa.(type) {
			case *net.IPAddr:
				if ifa.IP.Equal(devAddr.IP) {
					return true
				}
			case *net.IPNet:
				if ifa.IP.Equal(devAddr.IP) {
					return true
				}
			}
		}

		return false
	}

	for _, dev := range devices {
		for _, addr := range dev.Addresses {
			if len(dev.Name) > 0 && matchIfAddrFn(&addr) {
				devName = dev.Name
				break
			}
		}
		if len(devName) > 0 {
			break
		}
	}

	if len(devName) == 0 {
		err = errors.New("not found")
	}

	return
}
