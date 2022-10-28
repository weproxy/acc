//
// weproxy@foxmail.com 2022/10/28
//

package util

import (
	"bufio"
	"errors"
	"io"
	"net"
	"os"
	"os/exec"
	"runtime"
	"strings"
	"syscall"

	"github.com/google/gopacket"
	"github.com/google/gopacket/layers"
)

var ErrLinuxOnly = errors.New("linux only")

// Filer ...
type Filer interface {
	File() (*os.File, error)
}

// SetPromiscMode ...
func SetPromiscMode(ifName string, promisc bool) error {
	switch runtime.GOOS {
	case "linux":
		opt := "promisc"
		if !promisc {
			opt = "-promisc"
		}
		return exec.Command("ifconfig", ifName, opt).Run()
	case "darwin":
		opt := "1"
		if !promisc {
			opt = "0"
		}
		cmd := "/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport"
		return exec.Command(cmd, ifName, "sniff", opt).Run()
	}

	return errors.New("SetPromiscMode() not impl on " + runtime.GOOS)
}

// SetMulticastMode ...
func SetMulticastMode(ifName string, b bool) (err error) {
	switch runtime.GOOS {
	case "linux":
		opt := "multicast"
		if !b {
			opt = "-multicast"
		}
		return exec.Command("ifconfig", ifName, opt).Run()
	}

	return errors.New("SetMulticastMode() not impl on " + runtime.GOOS)
}

// GetDefaultInterface ...
func GetDefaultInterface(ifName string) (ifc *net.Interface, err error) {
	if len(ifName) > 0 {
		return net.InterfaceByName(ifName)
	}

	ifIdx, err := GetBestInterfaceIndex("8.8.8.8")
	if err != nil {
		return
	}

	return net.InterfaceByIndex(ifIdx)
}

// GetBridgesName ...
func GetBridgesName() (ifNames []string) {
	if runtime.GOOS != "linux" {
		return nil
	}

	const sysClassNet = "/sys/class/net/"

	ifaces := func() (arr []string) {
		files, _ := os.ReadDir(sysClassNet)
		for _, file := range files {
			arr = append(arr, file.Name())
		}
		return
	}()

	exists := func(path string) bool {
		_, err := os.Stat(path)
		return err == nil
	}

	for _, ifname := range ifaces {
		ifdir := sysClassNet + ifname
		if exists(ifdir+"/bridge/bridge_id") && exists(ifdir+"/bridge/stp_state") && exists(ifdir+"/brif") {
			if iface, err := net.InterfaceByName(ifname); err != nil {
				continue
			} else if (iface.Flags&net.FlagUp) == 0 || (iface.Flags&net.FlagPointToPoint) != 0 {
				continue
			} else if addrs, err := iface.Addrs(); err != nil || len(addrs) == 0 {
				continue
			}

			ifNames = append(ifNames, ifname)
		}
	}

	return
}

// checkAdapterIP ...
func checkAdapterIP(ifIdx int, ipnet net.IPNet) (ifc *net.Interface, exists bool, err error) {
	ifc, err = net.InterfaceByIndex(ifIdx)
	if err != nil {
		return
	}
	if addrs, err := ifc.Addrs(); err == nil {
		for _, ifa := range addrs {
			switch ifa := ifa.(type) {
			case *net.IPAddr:
				if ifa.IP.Equal(ipnet.IP) {
					return ifc, true, nil
				}
			case *net.IPNet:
				if ifa.IP.Equal(ipnet.IP) || ifa.Contains(ipnet.IP) {
					return ifc, true, nil
				}
			}
		}
	}

	return ifc, false, nil
}

// AddAdapterIP ...
func AddAdapterIP(ifIdx int, ipnet net.IPNet) (err error) {
	ifc, exists, err := checkAdapterIP(ifIdx, ipnet)
	if err != nil || exists {
		return
	}

	err = addOrDelAdapterIP(true, ifc, ipnet)
	if err == nil {
		// check again
		_, exists, err = checkAdapterIP(ifIdx, ipnet)
		if err == nil && !exists {
			err = errors.New("add adapter ip fail")
		}
	}

	return
}

// DelAdapterIP ...
func DelAdapterIP(ifIdx int, ipnet net.IPNet) (err error) {
	ifc, exists, err := checkAdapterIP(ifIdx, ipnet)
	if err != nil || !exists {
		return
	}

	err = addOrDelAdapterIP(false, ifc, ipnet)
	if err == nil {
		// check again
		_, exists, err = checkAdapterIP(ifIdx, ipnet)
		if err == nil && exists {
			err = errors.New("del adapter ip fail")
		}
	}

	return
}

// toIfidxAndIpnet ...
func toIfidxAndIpnet(ifname, cidr string) (ifIdx int, ipnet *net.IPNet, err error) {
	ip, ipnet, err := net.ParseCIDR(cidr)
	if err != nil {
		return
	}

	ifc, err := net.InterfaceByName(ifname)
	if err != nil {
		return
	}

	return ifc.Index, &net.IPNet{IP: ip, Mask: ipnet.Mask}, nil
}

// AddInterfaceIP -> AddInterfaceIP("tunAcc", "10.66.66.1/24")
func AddInterfaceIP(ifname, cidr string) (err error) {
	ifIdx, ipnet, err := toIfidxAndIpnet(ifname, cidr)
	if err != nil {
		return
	}

	return AddAdapterIP(ifIdx, *ipnet)
}

// DelInterfaceIP -> DelInterfaceIP("tunAcc", "10.66.66.1/24")
func DelInterfaceIP(ifname, cidr string) (err error) {
	ifIdx, ipnet, err := toIfidxAndIpnet(ifname, cidr)
	if err != nil {
		return
	}

	return DelAdapterIP(ifIdx, *ipnet)
}

// GetHostDefaultDNS ...
func GetHostDefaultDNS() (ips []net.IP) {
	if runtime.GOOS == "linux" || runtime.GOOS == "darwin" {
		// OSX scutil --dns

		file, err := os.Open("/etc/resolv.conf")
		if err == nil {
			defer file.Close()

			r := bufio.NewReader(file)
			for {
				line, err := r.ReadString('\n')
				if err != nil || err == io.EOF {
					break
				}
				line = strings.TrimSpace(line)
				if strings.HasPrefix(line, "nameserver ") {
					if ip := net.ParseIP(strings.TrimSpace(line[11:])); len(ip) > 0 {
						ips = append(ips, ip)
					}
				}
			}
		}
	}

	if len(ips) == 0 {
		return []net.IP{net.IPv4(223, 5, 5, 5)}
	}

	return
}

////////////////////////////////////////////////////////////////////////////////

// ForwardEthPacketFragment ..
func ForwardEthPacketFragment(eth *layers.Ethernet, ip4 *layers.IPv4, mtu int, forwardFn func([]byte) error) error {
	// We are not doing any sort of NAT, so we don't need to worry
	// about checksums of IP payload (eg UDP checksum).
	headerSize := int(ip4.IHL) * 4
	// &^ is bit clear (AND NOT). So here we're clearing the lowest 3
	// bits.
	maxSegmentSize := (mtu - headerSize) &^ 7

	opts := gopacket.SerializeOptions{
		FixLengths:       false,
		ComputeChecksums: true,
	}

	payloadSize := int(ip4.Length) - headerSize
	payload := ip4.BaseLayer.Payload[:payloadSize]
	offsetBase := int(ip4.FragOffset) << 3
	origFlags := ip4.Flags
	ip4.Flags = ip4.Flags | layers.IPv4MoreFragments
	ip4.Length = uint16(headerSize + maxSegmentSize)

	if eth.EthernetType == layers.EthernetTypeLLC {
		// using LLC, so must set eth length correctly. eth length
		// is just the length of the payload
		eth.Length = ip4.Length
	} else {
		eth.Length = 0
	}

	for offset := 0; offset < payloadSize; offset += maxSegmentSize {
		var segmentPayload []byte

		if len(payload) <= maxSegmentSize {
			// last one
			segmentPayload = payload
			ip4.Length = uint16(len(payload) + headerSize)
			ip4.Flags = origFlags
			if eth.EthernetType == layers.EthernetTypeLLC {
				eth.Length = ip4.Length
			} else {
				eth.Length = 0
			}
		} else {
			segmentPayload = payload[:maxSegmentSize]
			payload = payload[maxSegmentSize:]
		}

		ip4.FragOffset = uint16((offset + offsetBase) >> 3)
		buf := gopacket.NewSerializeBuffer()
		segPayload := gopacket.Payload(segmentPayload)

		err := gopacket.SerializeLayers(buf, opts, eth, ip4, &segPayload)
		if err != nil {
			return err
		}

		if err = forwardFn(buf.Bytes()); err != nil {
			return err
		}
	}
	return nil
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// SetTCPNoDelay controls whether the operating system should delay
// packet transmission in hopes of sending fewer packets (Nagle's algorithm).
//
// The default is true (no delay), meaning that data is
// sent as soon as possible after a Write.
func SetTCPNoDelay(fd, noDelay int) error {
	return os.NewSyscallError("setsockopt", syscall.SetsockoptInt(ConvFd(fd), syscall.IPPROTO_TCP, syscall.TCP_NODELAY, noDelay))
}

// GetRecvBuffer gets the size of the operating system's
// receive buffer associated with the connection.
func GetRecvBuffer(fd int) (size int, err error) {
	return syscall.GetsockoptInt(ConvFd(fd), syscall.SOL_SOCKET, syscall.SO_RCVBUF)
}

// SetRecvBuffer sets the size of the operating system's
// receive buffer associated with the connection.
func SetRecvBuffer(fd, size int) error {
	return os.NewSyscallError("setsockopt", syscall.SetsockoptInt(ConvFd(fd), syscall.SOL_SOCKET, syscall.SO_RCVBUF, size))
}

// GetSendBuffer gets the size of the operating system's
// transmit buffer associated with the connection.
func GetSendBuffer(fd int) (size int, err error) {
	return syscall.GetsockoptInt(ConvFd(fd), syscall.SOL_SOCKET, syscall.SO_SNDBUF)
}

// SetSendBuffer sets the size of the operating system's
// transmit buffer associated with the connection.
func SetSendBuffer(fd, size int) error {
	return os.NewSyscallError("setsockopt", syscall.SetsockoptInt(ConvFd(fd), syscall.SOL_SOCKET, syscall.SO_SNDBUF, size))
}

// SetBroadcast ...
func SetBroadcast(fd int, b int) error {
	return os.NewSyscallError("setsockopt", syscall.SetsockoptInt(ConvFd(fd), syscall.SOL_SOCKET, syscall.SO_BROADCAST, b))
}

///////////////////////////////////////////////////////////////////////////////

// BindToInterface ...
func BindToInterface(fd int, name string) error {
	iface, err := net.InterfaceByName(name)
	if err != nil {
		return err
	}

	return bindToInterface(fd, iface)
}
