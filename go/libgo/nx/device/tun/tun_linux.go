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
	cfg.PlatformSpecificParams.Name = c.name
	cfg.PlatformSpecificParams.Persist = c.persist

	dev, err := water.New(cfg)
	if err != nil {
		return nil, err
	}

	c.name = dev.Name()

	return dev, nil
}
