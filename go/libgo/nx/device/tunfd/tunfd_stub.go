package tunfd

import (
	"errors"
	"io"
)

// openTunDeviceWithFD ...
func openTunDeviceWithFD(tunFD int) (io.ReadWriteCloser, error) {
	return nil, errors.New("not support")
}
