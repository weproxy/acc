//
// weproxy@foxmail.com 2022/10/20
//

package proto

import (
	"encoding/json"

	"weproxy/acc/libgo/logx"
)

// TAG ...
const TAG = "[proto]"

// _protos/_servers ...
var (
	_protos  = make(map[string]NewServerFn)
	_servers = make(map[string]Server)
)

// Server ...
type Server interface {
	Start() error
	Close() error
}

// NewServerFn ...
type NewServerFn func(j json.RawMessage) (Server, error)

// Register ...
func Register(proto string, fn NewServerFn) {
	logx.D("%v Register(%s)", TAG, proto)
	_protos[proto] = fn
}

// Init ..
func Init(servers []json.RawMessage) error {
	logx.D("%v Init()", TAG)

	for _, j := range servers {
		var s struct {
			Proto    string `json:"proto,omitempty"`
			Disabled bool   `json:"disabled,omitempty"`
		}

		err := json.Unmarshal(j, &s)
		if err != nil || len(s.Proto) == 0 || s.Disabled {
			continue
		}

		if newFn, ok := _protos[s.Proto]; ok && newFn != nil {
			if svr, err := newFn(j); err != nil {
				return err
			} else if err = svr.Start(); err != nil {
				return err
			} else {
				_servers[s.Proto] = svr
			}
		}
	}

	return nil
}

// Deinit ...
func Deinit() {
	for _, svr := range _servers {
		svr.Close()
	}
	_servers = nil
	logx.D("%v Deinit()", TAG)
}
