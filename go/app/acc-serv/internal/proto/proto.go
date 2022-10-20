//
// weproxy@foxmail.com 2022/10/20
//

package proto

import "encoding/json"

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
	Close()
}

// NewServerFn ...
type NewServerFn func(j json.RawMessage) (Server, error)

// Register ...
func Register(proto string, fn NewServerFn) {
	_protos[proto] = fn
}

// Init ..
func Init(j json.RawMessage) error {
	return nil
}

// Deinit ...
func Deinit() {
}
