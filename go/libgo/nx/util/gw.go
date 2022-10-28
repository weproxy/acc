//
// weproxy@foxmail.com 2022/10/28
//

package util

import (
	"errors"
	"net"
	"os/exec"
	"runtime"
	"strings"
)

///////////////////////////////////////////////////////////////////////////////

// GetGateway ...
func GetGateway(dstIP string) (ifName, gwIP string, err error) {
	switch runtime.GOOS {
	case "linux":
		return getGatewayLinux(dstIP)
	case "darwin":
		return getGatewayDarwin(dstIP)
	case "windows":
		return getGatewayWindows(dstIP)
	case "freebsd", "solaris":
		return getGatewayFreeBSD(dstIP)
	}

	err = errNotImplemented
	return
}

///////////////////////////////////////////////////////////////////////////////

var (
	errNoGateway      = errors.New("no gateway found")
	errCantParse      = errors.New("can't parse string output")
	errNotImplemented = errors.New("not implemented for OS: " + runtime.GOOS)
)

// getGatewayLinux ...
func getGatewayLinux(dstIP string) (ifName, gwIP string, err error) {
	dat, err := exec.Command("ip", "-o", "route", "get", dstIP).Output()
	if err != nil {
		return
	}

	parts := strings.Fields(string(dat))
	for index, val := range parts {
		if val == "dev" && index < len(parts) {
			ifName = parts[index+1]
		} else if val == "via" && index < len(parts) {
			gwIP = parts[index+1]
		}
		if len(ifName) > 0 && len(gwIP) > 0 {
			break
		}
	}

	return
}

///////////////////////////////////////////////////////////////////////////////

// getGatewayDarwin ...
func getGatewayDarwin(dstIP string) (ifName, gwIP string, err error) {
	// route to: default
	// destination: default
	// 	   mask: default
	// 	gateway: 10.178.202.254
	//   interface: en0
	dat, err := exec.Command("/sbin/route", "-n", "get", dstIP).Output()
	if err != nil {
		return
	}

	lines := strings.Split(string(dat), "\n")
	for _, line := range lines {
		fields := strings.Fields(line)
		if len(fields) >= 2 && fields[0] == "interface:" {
			ifName = fields[1]
		}
		if len(fields) >= 2 && fields[0] == "gateway:" {
			if ip := net.ParseIP(fields[1]); ip != nil {
				gwIP = fields[1]
			}
		}
		if len(ifName) > 0 && len(gwIP) > 0 {
			break
		}
	}

	return
}

///////////////////////////////////////////////////////////////////////////////

type windowsRouteStruct struct {
	Destination string
	Netmask     string
	Gateway     string
	Interface   string
	Metric      string
}

// parseToWindowsRouteStruct ...
func parseToWindowsRouteStruct(output []byte) (windowsRouteStruct, error) {
	// Windows route output format is always like this:
	// ===========================================================================
	// Interface List
	// 8 ...00 12 3f a7 17 ba ...... Intel(R) PRO/100 VE Network Connection
	// 1 ........................... Software Loopback Interface 1
	// ===========================================================================
	// IPv4 Route Table
	// ===========================================================================
	// Active Routes:
	// Network Destination        Netmask          Gateway       Interface  Metric
	//           0.0.0.0          0.0.0.0      192.168.1.1    192.168.1.100     20
	// ===========================================================================
	//
	// Windows commands are localized, so we can't just look for "Active Routes:" string
	// I'm trying to pick the active route,
	// then jump 2 lines and get the row
	// Not using regex because output is quite standard from Windows XP to 8 (NEEDS TESTING)
	lines := strings.Split(string(output), "\n")
	sep := 0
	for idx, line := range lines {
		if sep == 3 {
			// We just entered the 2nd section containing "Active Routes:"
			if len(lines) <= idx+2 {
				return windowsRouteStruct{}, errNoGateway
			}

			fields := strings.Fields(lines[idx+2])
			if len(fields) < 5 {
				return windowsRouteStruct{}, errCantParse
			}

			return windowsRouteStruct{
				Destination: fields[0],
				Netmask:     fields[1],
				Gateway:     fields[2],
				Interface:   fields[3],
				Metric:      fields[4],
			}, nil
		}
		if strings.HasPrefix(line, "=======") {
			sep++
			continue
		}
	}
	return windowsRouteStruct{}, errNoGateway
}

// getGatewayWindows ...
func getGatewayWindows(dstIP string) (ifName, gwIP string, err error) {
	dat, err := exec.Command("route", "print", "-4", "0.0.0.0").Output()
	if err != nil {
		return
	}

	ret, err := parseToWindowsRouteStruct(dat)
	if err != nil {
		return
	}

	ifName = ret.Interface
	gwIP = ret.Gateway

	if net.ParseIP(ifName) != nil {
		if ifIdx, err := GetBestInterfaceIndex(dstIP); err == nil {
			if ifc, err := net.InterfaceByIndex(ifIdx); err == nil {
				ifName = ifc.Name
				// gwIP = ifc.Addrs()
			}
		}
	}

	return
}

///////////////////////////////////////////////////////////////////////////////

// getGatewayFreeBSD ...
func getGatewayFreeBSD(dstIP string) (ifName, gwIP string, err error) {
	// netstat -rn produces the following on FreeBSD:
	// Routing tables
	//
	// Internet:
	// Destination        Gateway            Flags      Netif Expire
	// default            10.88.88.2         UGS         em0
	// 10.88.88.0/24      link#1             U           em0
	// 10.88.88.148       link#1             UHS         lo0
	// 127.0.0.1          link#2             UH          lo0
	//
	// Internet6:
	// Destination                       Gateway                       Flags      Netif Expire
	// ::/96                             ::1                           UGRS        lo0
	// ::1                               link#2                        UH          lo0
	// ::ffff:0.0.0.0/96                 ::1                           UGRS        lo0
	// fe80::/10                         ::1                           UGRS        lo0
	// ...
	dat, err := exec.Command("netstat", "-rn").Output()
	if err != nil {
		return
	}

	outputLines := strings.Split(string(dat), "\n")
	for _, line := range outputLines {
		fields := strings.Fields(line)
		if len(fields) >= 3 && fields[0] == "default" {
			ip := net.ParseIP(fields[1])
			if ip != nil {
				ifName = fields[3]
				gwIP = fields[1]
				return
			}
		}
	}

	err = errNoGateway

	return
}
