package tun

import (
	"errors"
	"fmt"
	"io"
	"net"
	"strconv"

	"github.com/songgao/water"
)

// tunDev ...
type tunDev struct {
	*water.Interface
	c *conf
}

// Close ...
func (m *tunDev) Close() error {
	if m.c != nil {
		shellExec("route %v default %v -ifscope %v", "del", m.c.gw, m.c.name)
	}
	if m.Interface != nil {
		return m.Interface.Close()
	}
	return nil
}

// openTunDevice ...
func openTunDevice(c *conf) (io.ReadWriteCloser, error) {
	cfg := water.Config{
		DeviceType: water.TUN,
	}

	dev, err := water.New(cfg)
	if err != nil {
		return nil, err
	}

	c.name = dev.Name()

	if ip := net.ParseIP(c.addr); ip == nil {
		return nil, errors.New("invalid IP address")
	} else if ip.To4() != nil {
		err = shellExec("ifconfig %s inet %s netmask %s %s", c.name, c.addr, c.mask, c.gw)
	} else if ip.To16() != nil {
		prefixlen, er2 := strconv.Atoi(c.mask)
		if er2 != nil {
			return nil, fmt.Errorf("parse IPv6 prefixlen failed: %v", err)
		}
		err = shellExec("ifconfig %s inet6 %s/%d", c.name, c.addr, prefixlen)
	} else {
		return nil, errors.New("invalid IP address")
	}
	if err != nil {
		return nil, err
	}

	shellExec("route %v default %v -ifscope %v", "add", c.gw, c.name)

	return &tunDev{c: c, Interface: dev}, nil
}
