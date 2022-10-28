//
// weproxy@foxmail.com 2022/10/28
//

package util

import (
	"errors"
	"fmt"
	"net"
	"os"
	"os/exec"
	"strings"
	"syscall"
	"unsafe"
)

// ConvFd ...
func ConvFd(fd int) int { return fd }

// SetKeepAlive sets whether the operating system should send
// keep-alive messages on the connection and sets period between keep-alive's.
func SetKeepAlive(fd, secs int) error {
	if secs <= 0 {
		return errors.New("invalid time duration")
	}
	if err := os.NewSyscallError("setsockopt", syscall.SetsockoptInt(fd, syscall.SOL_SOCKET, syscall.SO_KEEPALIVE, 1)); err != nil {
		return err
	}
	if err := os.NewSyscallError("setsockopt", syscall.SetsockoptInt(fd, syscall.IPPROTO_TCP, syscall.TCP_KEEPINTVL, secs)); err != nil {
		return err
	}
	return os.NewSyscallError("setsockopt", syscall.SetsockoptInt(fd, syscall.IPPROTO_TCP, syscall.TCP_KEEPIDLE, secs))
}

func htons(i uint16) uint16 {
	return (i<<8)&0xff00 | i>>8
}

// CreateRawArpSocket ...
func CreateRawArpSocket() (fd int, err error) {
	return syscall.Socket(syscall.AF_PACKET, syscall.SOCK_RAW, int(htons(syscall.ETH_P_ARP)))
}

// CreateRawSocket ...
func CreateRawSocket(ethMode bool) (fd int, err error) {
	if ethMode {
		return syscall.Socket(syscall.AF_PACKET, syscall.SOCK_RAW, int(htons(syscall.ETH_P_ALL)))
		// return syscall.Socket(syscall.AF_PACKET, syscall.SOCK_RAW, syscall.ETH_P_ALL)
	}

	fd, err = syscall.Socket(syscall.AF_INET, syscall.SOCK_RAW, syscall.IPPROTO_RAW)
	if err == nil {
		// IP_HDRINC=1 means user set IP head fields
		err = syscall.SetsockoptInt(fd, syscall.IPPROTO_IP, syscall.IP_HDRINCL, 1)
		if err != nil {
			err = os.NewSyscallError("setsockopt socket IP_HDRINCL", err)
			syscall.Close(fd)
			return 0, err
		}
	}

	return
}

// GetPromiscuous ..
func GetPromiscuous(i net.Interface) (bool, error) {
	tab, err := syscall.NetlinkRIB(syscall.RTM_GETLINK, syscall.AF_UNSPEC)
	if err != nil {
		return false, os.NewSyscallError("netlinkrib", err)
	}
	msgs, err := syscall.ParseNetlinkMessage(tab)
	if err != nil {
		return false, os.NewSyscallError("parsenetlinkmessage", err)
	}
loop:
	for _, m := range msgs {
		switch m.Header.Type {
		case syscall.NLMSG_DONE:
			break loop
		case syscall.RTM_NEWLINK:
			ifim := (*syscall.IfInfomsg)(unsafe.Pointer(&m.Data[0]))
			if ifim.Index == int32(i.Index) {
				return (ifim.Flags & syscall.IFF_PROMISC) != 0, nil
			}
		}
	}
	return false, os.ErrNotExist
}

// addOrDelAdapterIP ...
func addOrDelAdapterIP(isAdd bool, ifc *net.Interface, ipnet net.IPNet) (err error) {
	// ifconfig en0 add 10.66.66.1/24
	opt := "add"
	if !isAdd {
		opt = "del"
	}

	// ip addr add 10.66.66.1/24 brd + dev en0:0
	// ip addr del 10.66.66.1/24 dev en0:0
	cmd := fmt.Sprintf("ip addr %s %s dev %v:0", opt, ipnet.String(), ifc.Name)
	println(cmd)
	arr := strings.Split(cmd, " ")

	b, err := exec.Command(arr[0], arr[1:]...).CombinedOutput()
	if err != nil && len(b) > 0 {
		err = errors.New(string(b))
	}

	return err
}

// GetBestInterfaceIndex is ...
func GetBestInterfaceIndex(dstIP string) (ifIdx int, err error) {
	ifname, _, err := GetGateway(dstIP)
	if err != nil {
		return
	}

	iface, err := net.InterfaceByName(ifname)
	if err == nil {
		ifIdx = iface.Index
	}

	return
}

// bindToInterface ...
func bindToInterface(fd int, iface *net.Interface) error {
	return os.NewSyscallError("BindToDevice", syscall.BindToDevice(fd, iface.Name))
}
