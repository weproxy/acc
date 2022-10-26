//
// weproxy@foxmail.com 2022/10/20
//

package eth

import (
	"errors"
	"io"

	"weproxy/acc/libgo/nx/device"
	"weproxy/acc/libgo/nx/stack/netstk"
)

// init ...
func init() {
	device.Register(device.TypeRAW, NewDevice)
}

// NewDevice ...
func NewDevice(cfg map[string]interface{}) (netstk.Device, error) {
	return nil, errors.New("eth.NewDevice() not impl")
}

////////////////////////////////////////////////////////////////////////////////

// Device implements netstk.Device
type Device struct {
}

// Type ...
func (m *Device) Type() string {
	return device.TypeTUN
}

// Close implements io.Closer
func (m *Device) Close() error {
	return nil
}

// Read from device
func (m *Device) Read(p []byte, offset int) (n int, err error) {
	return 0, io.EOF
}

// Write to device
func (m *Device) Write(p []byte, offset int) (n int, err error) {
	return 0, io.EOF
}
