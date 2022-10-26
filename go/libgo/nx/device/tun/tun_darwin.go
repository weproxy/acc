package tun

import (
	"errors"
	"fmt"
	"io"
	"net"
	"os/exec"
	"strconv"
	"strings"

	"github.com/songgao/water"
)

// openTunDevice ...
func openTunDevice(c *conf) (io.ReadWriteCloser, error) {
	cfg := water.Config{
		DeviceType: water.TUN,
	}

	tunDev, err := water.New(cfg)
	if err != nil {
		return nil, err
	}

	c.name = tunDev.Name()

	ip := net.ParseIP(c.addr)
	if ip == nil {
		return nil, errors.New("invalid IP address")
	}

	var params string
	if ip.To4() != nil {
		params = fmt.Sprintf("%s inet %s netmask %s %s", c.name, c.addr, c.mask, c.gw)
	} else if ip.To16() != nil {
		prefixlen, err := strconv.Atoi(c.mask)
		if err != nil {
			return nil, fmt.Errorf("parse IPv6 prefixlen failed: %v", err)
		}
		params = fmt.Sprintf("%s inet6 %s/%d", c.name, c.addr, prefixlen)
	} else {
		return nil, errors.New("invalid IP address")
	}

	out, err := exec.Command("ifconfig", strings.Split(params, " ")...).Output()

	if err != nil {
		if len(out) != 0 {
			return nil, fmt.Errorf("%v, output: %s", err, out)
		}
		return nil, err
	}

	return tunDev, nil
}
