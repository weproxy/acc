//
// weproxy@foxmail.com 2022/10/20
//

package device

import (
	"errors"
	"strings"

	"weproxy/acc/libgo/nx/stack/netstk"
)

const (
	TypePCAP  = "pcap"
	TypeRAW   = "raw"
	TypeTUN   = "tun"
	TypeTUNFD = "tunfd"
	TypeWFP   = "wfp"
)

var (
	_devs = make(map[string]NewDeviceFn)
)

// NewDeviceFn ..
type NewDeviceFn func(cfg map[string]interface{}) (netstk.Device, error)

// Register ...
func Register(typ string, fn NewDeviceFn) {
	_devs[typ] = fn
}

// New ...
func New(typ string, cfg map[string]interface{}) (netstk.Device, error) {
	if fn, ok := _devs[strings.ToLower(typ)]; ok && fn != nil {
		return fn(cfg)
	}
	return nil, errors.New("device.New() unkown type: " + typ)
}
