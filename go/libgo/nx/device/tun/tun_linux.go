package tun

import (
	"io"

	"github.com/songgao/water"
)

// openTunDevice ...
func openTunDevice(c *conf) (io.ReadWriteCloser, error) {
	cfg := water.Config{
		Name:       c.name,
		Persist:    c.persist,
		DeviceType: water.TUN,
	}

	dev, err := water.New(cfg)
	if err != nil {
		return nil, err
	}

	c.name = dev.Name()

	return dev, nil
}
