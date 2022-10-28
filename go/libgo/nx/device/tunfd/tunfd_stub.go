//
// weproxy@foxmail.com 2022/10/20
//

//go:build !linux
// +build !linux

package tunfd

import (
	"errors"
	"io"
)

// openTunDeviceWithFD ...
func openTunDeviceWithFD(tunFD int, persist bool) (io.ReadWriteCloser, error) {
	return nil, errors.New("tunfd not support on this platform")
}
