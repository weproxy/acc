package tunfd

import (
	"io"

	"github.com/songgao/water"
)

// openTunDeviceWithFD ...
func openTunDeviceWithFD(tunFD int) (io.ReadWriteCloser, error) {
	cfg := water.Config{
		DeviceType: water.TUN,
		TunFD:      tunFD,
	}
	cfg.Persist = persist

	tunDev, err := water.New(cfg)
	if err != nil {
		return nil, err
	}

	return tunDev, nil
}
