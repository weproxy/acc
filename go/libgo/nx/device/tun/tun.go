//
// weproxy@foxmail.com 2022/10/20
//

package tun

import (
	"errors"
	"fmt"
	"io"
	"net"
	"os/exec"
	"strings"

	"weproxy/acc/libgo/nx/device"
	"weproxy/acc/libgo/nx/stack/netstk"
)

// init ...
func init() {
	device.Register(device.TypeTUN, New)
}

// conf ...
type conf struct {
	name           string
	addr, gw, mask string
	dns            []string
	persist        bool
}

// New ...
func New(cfg device.Conf) (netstk.Device, error) {
	c := &conf{
		name:    "utun",
		addr:    "10.6.6.2",
		gw:      "10.6.6.1",
		mask:    "255.255.255.0",
		dns:     nil,
		persist: false,
	}

	if s := cfg.Str("ifname"); len(s) > 0 {
		c.name = s
	}
	if s := cfg.Str("cidr"); len(s) > 0 {
		if ip, _, err := net.ParseCIDR(s); err == nil {
			c.addr, c.gw = ip.String(), ip.String()
		}
	}

	rwc, err := openTunDevice(c)
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
	return device.TypeTUN
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

////////////////////////////////////////////////////////////////////////////////

// shellExec ...
func shellExec(format string, args ...any) (err error) {
	str := fmt.Sprintf(format, args...)
	arr := strings.Split(str, " ")
	if len(arr) < 1 {
		return errors.New("empty cmd")
	}

	out, err := exec.Command(arr[0], arr[1:]...).Output()
	if err != nil {
		if len(out) != 0 {
			err = fmt.Errorf("%v, output: %s", err, out)
		}
		return
	}

	return
}
