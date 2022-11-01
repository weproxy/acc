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

	"golang.org/x/sys/unix"
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
	switch err := os.NewSyscallError("setsockopt", syscall.SetsockoptInt(fd, syscall.IPPROTO_TCP, unix.TCP_KEEPINTVL, secs)); err {
	case nil, syscall.ENOPROTOOPT: // OS X 10.7 and earlier don't support this option
	default:
		return err
	}
	return os.NewSyscallError("setsockopt", syscall.SetsockoptInt(fd, syscall.IPPROTO_TCP, syscall.TCP_KEEPALIVE, secs))
}

// CreateRawSocket ...
func CreateRawSocket(ethMode bool) (fd int, err error) {
	return 0, errors.New("CreateRawSocket() not impl")
}

// CreateRawArpSocket ...
func CreateRawArpSocket() (fd int, err error) {
	// return syscall.Socket(syscall.AF_PACKET, syscall.SOCK_RAW, int(htons(syscall.ETH_P_ARP)))
	return 0, errors.New("CreateRawArpSocket() not impl")
}

// addOrDelAdapterIP ...
func addOrDelAdapterIP(isAdd bool, ifc *net.Interface, ipnet net.IPNet) (err error) {
	opt := "alias"
	if !isAdd {
		opt = "-alias"
	}

	// ifconfig en0 inet 10.66.66.1 netmask 255.255.255.0 alias
	// ifconfig en0 inet 10.66.66.1 netmask 255.255.255.0 -alias
	cmd := fmt.Sprintf("ifconfig %s inet %v netmask %s %s", ifc.Name, ipnet.IP, net.IP(ipnet.Mask), opt)
	println(cmd)
	arr := strings.Split(cmd, " ")

	b, err := exec.Command(arr[0], arr[1:]...).CombinedOutput()
	if err != nil && len(b) > 0 {
		err = errors.New(string(b))
	}

	return err
}

// GetBestInterfaceIndex ...
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
func bindToInterface(fd int, i *net.Interface) error {
	// network := "tcp4"

	// switch network {
	// case "tcp4", "udp4":
	return os.NewSyscallError("setsockopt", syscall.SetsockoptInt(fd, syscall.IPPROTO_IP, syscall.IP_BOUND_IF, i.Index)) // unix.IP_RECVIF on BSD
	// case "tcp6", "udp6":
	// 	return os.NewSyscallError("setsockopt", syscall.SetsockoptInt(fd, syscall.IPPROTO_IPV6, syscall.IPV6_BOUND_IF, i.Index))
	// }

	// return errors.New("unsupport network")
}

// SetMark ..
func SetMark(fd int, mark int) error {
	return ErrLinuxOnly
}

// SetReuseport enables SO_REUSEPORT option on socket.
func SetReuseport(fd, reusePort int) error {
	return ErrLinuxOnly
}

// SetOptTransparent set socket with ip transparent option
func SetOptTransparent(fd, opt int) (err error) {
	//
	return ErrLinuxOnly
}

// SetOptRecvOrigDst set socket with recv origin dst option
func SetOptRecvOrigDst(fd, opt int) (err error) {
	return ErrLinuxOnly
}

// GetTCPOriginDst ...
func GetTCPOriginDst(c *net.TCPConn) (*net.TCPAddr, error) {
	return nil, ErrLinuxOnly
}

// GetUDPOriginDst ...
func GetUDPOriginDst(hdr []byte) (*net.UDPAddr, error) {
	return nil, ErrLinuxOnly
}
