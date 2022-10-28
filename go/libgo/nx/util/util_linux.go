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

// SetMark ..
func SetMark(fd int, mark int) error {
	return os.NewSyscallError("setsockopt", syscall.SetsockoptInt(fd, syscall.SOL_SOCKET, syscall.SO_MARK, mark))
}

// SetReuseport enables SO_REUSEPORT option on socket.
func SetReuseport(fd, reusePort int) error {
	if err := os.NewSyscallError("setsockopt", syscall.SetsockoptInt(ConvFd(fd), syscall.SOL_SOCKET, syscall.SO_REUSEADDR, reusePort)); err != nil {
		return err
	}
	return os.NewSyscallError("setsockopt", syscall.SetsockoptInt(ConvFd(fd), syscall.SOL_SOCKET, syscall.SO_REUSEPORT, reusePort))
}

const (
	// SOL_IP          = 0
	SO_ORIGINAL_DST = 80
)

// SetTCPOptRecvOrigDst set socket with ip transparent option
func SetTCPOptRecvOrigDst(c Filer) (err error) {
	file, err := c.File()
	if err != nil {
		return err
	}
	fd := int(file.Fd())
	defer file.Close()

	return SetOptTransparent(fd, 1)
}

// SetUDPOptRecvOrigDst set socket with ip transparent option
func SetUDPOptRecvOrigDst(c Filer) (err error) {
	file, err := c.File()
	if err != nil {
		return err
	}
	fd := int(file.Fd())
	defer file.Close()

	err = SetOptTransparent(fd, 1)
	if err != nil {
		return err
	}

	err = SetOptRecvOrigDst(fd, 1)
	if err != nil {
		return err
	}

	return nil
}

// SetOptTransparent ...
func SetOptTransparent(fd, opt int) (err error) {
	// set socket with ip transparent option
	return syscall.SetsockoptInt(fd, syscall.SOL_IP, syscall.IP_TRANSPARENT, opt)
}

// SetOptRecvOrigDst ...
func SetOptRecvOrigDst(fd, opt int) (err error) {
	// set socket with recv origin dst option
	return syscall.SetsockoptInt(fd, syscall.SOL_IP, syscall.IP_RECVORIGDSTADDR, opt)
}

// GetTCPOriginDst 获取 netfilter 在 REDIRECT 之前的原始目标地址
func GetTCPOriginDst(c *net.TCPConn) (*net.TCPAddr, error) {
	f, err := c.File()
	if err != nil {
		return nil, err
	}
	fd := int(f.Fd())
	defer f.Close()

	ipv6Mreq, err := syscall.GetsockoptIPv6Mreq(fd, syscall.SOL_IP, SO_ORIGINAL_DST)
	if err != nil {
		addr := c.LocalAddr()
		return net.ResolveTCPAddr(addr.Network(), addr.String())
	}

	ip := net.IP(ipv6Mreq.Multiaddr[4:8])
	port := int(binary.BigEndian.Uint16(ipv6Mreq.Multiaddr[2:4]))

	return &net.TCPAddr{IP: ip, Port: port}, nil
}

var ErrGetOriginDstFail = errors.New("get origin dst fail")

// GetUDPOriginDst ...
func GetUDPOriginDst(hdr []byte) (dst *net.UDPAddr, err error) {
	msgs, err := syscall.ParseSocketControlMessage(hdr)
	if err != nil {
		return
	}

	for _, msg := range msgs {
		if msg.Header.Level == syscall.SOL_IP && msg.Header.Type == syscall.IP_RECVORIGDSTADDR {
			raw := &syscall.RawSockaddrInet4{}

			err := binary.Read(bytes.NewReader(msg.Data), binary.LittleEndian, raw)
			if err != nil {
				continue
			}

			// only support for ipv4
			if raw.Family == syscall.AF_INET {
				pp := (*syscall.RawSockaddrInet4)(unsafe.Pointer(raw))
				p := (*[2]byte)(unsafe.Pointer(&pp.Port))

				dst = &net.UDPAddr{
					IP:   net.IPv4(pp.Addr[0], pp.Addr[1], pp.Addr[2], pp.Addr[3]),
					Port: int(p[0])<<8 + int(p[1]),
				}
				return dst, nil
			}
		}
	}

	if dst == nil {
		err = ErrGetOriginDstFail
	}

	return
}
