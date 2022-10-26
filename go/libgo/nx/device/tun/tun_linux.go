package tun

import (
	"io"

	"github.com/songgao/water"
)

// openTunDevice ...
func openTunDevice(c *conf) (io.ReadWriteCloser, error) {
	cfg := water.Config{
		DeviceType: water.TUN,
	}
	cfg.Name = c.name
	cfg.Persist = c.persist

	tunDev, err := water.New(cfg)
	if err != nil {
		return nil, err
	}

	c.name = tunDev.Name()

	return tunDev, nil
}
