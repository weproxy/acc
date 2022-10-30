//
// weproxy@foxmail.com 2022/10/20
//

package device

import (
	"errors"
	"fmt"
	"strconv"
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
type NewDeviceFn func(cfg Conf) (netstk.Device, error)

// Register ...
func Register(typ string, fn NewDeviceFn) {
	_devs[typ] = fn
}

// New ...
func New(typ string, cfg Conf) (netstk.Device, error) {
	if fn, ok := _devs[strings.ToLower(typ)]; ok && fn != nil {
		return fn(cfg)
	}
	return nil, errors.New("device.New() unkown type: " + typ)
}

////////////////////////////////////////////////////////////////////////////////

// Conf ...
type Conf map[string]any

// Int ...
func (m Conf) Int(key string) int {
	if m != nil {
		if v, ok := m[key]; ok {
			if n, ok := v.(int); ok {
				return n
			} else if s, ok := v.(string); ok {
				if n, e := strconv.Atoi(s); e == nil {
					return n
				}
			}
		}
	}
	return 0
}

// Str ...
func (m Conf) Str(key string) string {
	if m != nil {
		if v, ok := m[key]; ok {
			if s, ok := v.(string); ok {
				return s
			} else {
				return fmt.Sprintf("%v", v)
			}
		}
	}
	return ""
}
