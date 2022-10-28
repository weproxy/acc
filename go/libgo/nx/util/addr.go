//
// weproxy@foxmail.com 2022/10/28
//

package util

import (
	"net"
	"syscall"
)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NetToSockAddr ...
func NetToSockAddr(addr net.Addr) (sa syscall.Sockaddr) {
	if addr == nil {
		return nil
	}

	switch addr := addr.(type) {
	case *net.IPAddr:
		return IPPortToSockAddr(addr.IP, 0, addr.Zone)
	case *net.TCPAddr:
		return IPPortToSockAddr(addr.IP, addr.Port, addr.Zone)
	case *net.UDPAddr:
		return IPPortToSockAddr(addr.IP, addr.Port, addr.Zone)
	case *net.UnixAddr:
		return &syscall.SockaddrUnix{Name: addr.Name}
	default:
		return nil
	}
}

// IPPortToSockAddr ...
func IPPortToSockAddr(ip net.IP, port int, zone string) (sa syscall.Sockaddr) {
	if ip4 := ip.To4(); ip4 != nil {
		m := &syscall.SockaddrInet4{Port: port}
		copy(m.Addr[:], ip4)
		return m
	} else if ip6 := ip.To16(); ip6 != nil {
		m := &syscall.SockaddrInet6{Port: port}
		copy(m.Addr[:], ip6)
		if zone != "" {
			var iface *net.Interface
			iface, err := net.InterfaceByName(zone)
			if err != nil {
				return
			}

			m.ZoneId = uint32(iface.Index)
		}
		return m
	}
	return nil
}

// SockToTCPAddr converts a Sockaddr to a net.TCPAddr
func SockToTCPAddr(sa syscall.Sockaddr) net.Addr {
	switch sa := sa.(type) {
	case *syscall.SockaddrInet4:
		ip := sockaddrInet4ToIP(sa)
		return &net.TCPAddr{IP: ip, Port: sa.Port}
	case *syscall.SockaddrInet6:
		ip, zone := sockaddrInet6ToIPAndZone(sa)
		return &net.TCPAddr{IP: ip, Port: sa.Port, Zone: zone}
	}
	return nil
}

// SockToUDPAddr converts a Sockaddr to a net.UDPAddr
func SockToUDPAddr(sa syscall.Sockaddr) net.Addr {
	switch sa := sa.(type) {
	case *syscall.SockaddrInet4:
		ip := sockaddrInet4ToIP(sa)
		return &net.UDPAddr{IP: ip, Port: sa.Port}
	case *syscall.SockaddrInet6:
		ip, zone := sockaddrInet6ToIPAndZone(sa)
		return &net.UDPAddr{IP: ip, Port: sa.Port, Zone: zone}
	}
	return nil
}

// sockaddrInet4ToIPAndZone converts a SockaddrInet4 to a net.IP.
func sockaddrInet4ToIP(sa *syscall.SockaddrInet4) net.IP {
	ip := make([]byte, 16)
	// V4InV6Prefix
	ip[10] = 0xff
	ip[11] = 0xff
	copy(ip[12:16], sa.Addr[:])
	return ip
}

// sockaddrInet6ToIPAndZone converts a SockaddrInet6 to a net.IP with IPv6 Zone.
func sockaddrInet6ToIPAndZone(sa *syscall.SockaddrInet6) (net.IP, string) {
	ip := make([]byte, 16)
	copy(ip, sa.Addr[:])
	return ip, ip6ZoneToString(int(sa.ZoneId))
}

// ip6ZoneToString converts an IP6 Zone syscall int to a net string
func ip6ZoneToString(zone int) string {
	if zone == 0 {
		return ""
	}
	if ifi, err := net.InterfaceByIndex(zone); err == nil {
		return ifi.Name
	}
	return int2decimal(uint(zone))
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Convert int to decimal string.
func int2decimal(i uint) string {
	if i == 0 {
		return "0"
	}

	// Assemble decimal in reverse order.
	var b [32]byte
	bp := len(b)
	for ; i > 0; i /= 10 {
		bp--
		b[bp] = byte(i%10) + '0'
	}
	return string(b[bp:])
}
