//
// weproxy@foxmail.com 2022/10/20
//

package tunfd

import (
	"errors"
	"io"

	"weproxy/acc/libgo/nx/device"
	"weproxy/acc/libgo/nx/stack/netstk"
)

// init ...
func init() {
	device.Register(device.TypeTUNFD, New)
}

// New ...
func New(cfg device.Conf) (netstk.Device, error) {
	tunfd, ok := func() (int, bool) {
		if cfg != nil {
			if v, ok := cfg["tunfd"]; ok {
				if fd, ok := v.(int); ok && fd > 0 {
					return fd, true
				}
			}
		}
		return 0, false
	}()
	if !ok {
		return nil, errors.New("tunfd.New() err: missing cfg[\"tunfd\"]")
	}

	rwc, err := openTunDeviceWithFD(tunfd, false)
	if err != nil {
		return nil, err
	}

	dev := &Device{rwc: rwc}

	return dev, nil
}

////////////////////////////////////////////////////////////////////////////////

// Device implements netstk.Device
type Device struct {
	rwc io.ReadWriteCloser
}

// Type ...
func (m *Device) Type() string {
	return device.TypeTUNFD
}

// Close implements io.Closer
func (m *Device) Close() error {
	return m.rwc.Close()
}

// Read from device
func (m *Device) Read(p []byte, offset int) (n int, err error) {
	return m.rwc.Read(p[offset:])
}

// Write to device
func (m *Device) Write(p []byte, offset int) (n int, err error) {
	return m.rwc.Write(p[offset:])
}
