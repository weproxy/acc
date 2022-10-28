//
// weproxy@foxmail.com 2022/10/28
//

package util

import (
	"errors"
	"net"
	"net/netip"
	"strings"
	"syscall"
	"unsafe"

	"golang.org/x/sys/windows"

	"golang.zx2c4.com/wireguard/windows/tunnel/winipcfg"
)

var (
	iphlpapi = windows.MustLoadDLL("iphlpapi.dll")
	// getExtendedTcpTable = iphlpapi.MustFindProc("GetExtendedTcpTable")
	// getExtendedUdpTable = iphlpapi.MustFindProc("GetExtendedUdpTable")
	getBestInterfaceEx = iphlpapi.MustFindProc("GetBestInterfaceEx")
)

////////////////////////////////////////////////////////////////////////////////////////////////////

// ConvFd ...
func ConvFd(fd int) syscall.Handle { return syscall.Handle(fd) }

////////////////////////////////////////////////////////////////////////////////////////////////////

// GetBestInterfaceIndex is ...
func GetBestInterfaceIndex(dstIP string) (ifIdx int, err error) {
	destAddr := windows.RawSockaddr{}

	ip := net.ParseIP(dstIP)
	if ip == nil {
		return 0, errors.New("parse ip error")
	}
	if ipv4 := ip.To4(); ipv4 != nil {
		addr := (*windows.RawSockaddrInet4)(unsafe.Pointer(&destAddr))
		addr.Family = windows.AF_INET
		copy(addr.Addr[:], ipv4)
	} else {
		ipv6 := ip.To16()
		addr := (*windows.RawSockaddrInet6)(unsafe.Pointer(&destAddr))
		addr.Family = windows.AF_INET6
		copy(addr.Addr[:], ipv6)
	}

	return getBestInterface(&destAddr)
}

// getBestInterface is ...
func getBestInterface(addr *windows.RawSockaddr) (ifIdx int, err error) {
	dwBestIfIndex := int32(0)

	// IPHLPAPI_DLL_LINKAGE DWORD GetBestInterfaceEx(
	//  sockaddr *pDestAddr,
	//  PDWORD   pdwBestIfIndex
	// );
	// https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getbestinterfaceex
	ret, _, errno := getBestInterfaceEx.Call(
		uintptr(unsafe.Pointer(addr)),
		uintptr(unsafe.Pointer(&dwBestIfIndex)),
	)
	if ret == windows.NO_ERROR {
		return int(dwBestIfIndex), nil
	}
	return 0, errno
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// addOrDelAdapterIP ...
func addOrDelAdapterIP(isAdd bool, ifc *net.Interface, ipnet net.IPNet) (err error) {
	prefix, err := netip.ParsePrefix(ipnet.String())
	if err != nil {
		return
	}

	addrs, err := winipcfg.GetAdaptersAddresses(windows.AF_INET, winipcfg.GAAFlagDefault)
	if err != nil {
		return
	}

	for _, addr := range addrs {
		if addr.IfIndex != uint32(ifc.Index) {
			continue
		}

		if isAdd {
			if err = addr.LUID.AddIPAddress(prefix); err != nil {
				if strings.Contains(err.Error(), "already exists") {
					return nil
				}
				return err
			}
		} else {
			if err = addr.LUID.DeleteIPAddress(prefix); err != nil {
				return err
			}
		}
		return nil
	}

	return errors.New("not found")
}
