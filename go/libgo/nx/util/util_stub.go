//
// weproxy@foxmail.com 2022/10/28
//

//go:build !linux && !darwin
// +build !linux,!darwin

package util

import (
	"errors"
	"net"
)

// // ConvFd ...
// func ConvFd(fd int) int { return fd }

// bindToInterface ...
func bindToInterface(fd int, i *net.Interface) error {
	return ErrLinuxOnly
}

// SetReuseport enables SO_REUSEPORT option on socket.
func SetReuseport(fd, reusePort int) error {
	return ErrLinuxOnly
}

// SetOptTransparent ...
func SetOptTransparent(fd, opt int) (err error) {
	// set socket with ip transparent option
	return ErrLinuxOnly
}

// SetOptRecvOrigDst ...
func SetOptRecvOrigDst(fd, opt int) (err error) {
	// set socket with recv origin dst option
	return ErrLinuxOnly
}

// GetTCPOriginDst 获取 netfilter 在 REDIRECT 之前的原始目标地址
func GetTCPOriginDst(c *net.TCPConn) (*net.TCPAddr, error) {
	return nil, ErrLinuxOnly
}

// GetUDPOriginDst ...
func GetUDPOriginDst(hdr []byte) (*net.UDPAddr, error) {
	return nil, ErrLinuxOnly
}

// MakeUDPOriginDstOOB ...
func MakeUDPOriginDstOOB(addr *net.UDPAddr) (oob []byte, err error) {
	return nil, ErrLinuxOnly
}

// CreateRawArpSocket ...
func CreateRawArpSocket() (fd int, err error) {
	return 0, ErrLinuxOnly
}

// CreateRawSocket ...
func CreateRawSocket(ethMode bool) (fd int, err error) {
	return 0, ErrLinuxOnly
}

// SendUDPViaRawETH ...
func SendUDPViaRawETH(fd int, srcIface *net.Interface, dstMAC net.HardwareAddr, src, dst *net.UDPAddr, payload []byte) (err error) {
	return errors.New("not support")
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// // addOrDelAdapterIP ...
// func addOrDelAdapterIP(isAdd bool, ifc *net.Interface, ipnet net.IPNet) (err error) {
// 	return errors.New("not support")
// }
