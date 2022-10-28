package tunfd

import (
	"io"

	"github.com/songgao/water"
)

// openTunDeviceWithFD ...
func openTunDeviceWithFD(tunFD int, persist bool) (io.ReadWriteCloser, error) {
	cfg := water.Config{
		DeviceType: water.TUN,
		TunFD:      tunFD,
	}
	cfg.Persist = persist

	dev, err := water.New(cfg)
	if err != nil {
		return nil, err
	}

	return dev, nil
}
