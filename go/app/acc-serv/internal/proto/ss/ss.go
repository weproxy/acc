//
// weproxy@foxmail.com 2022/10/20
//

package ss

import (
	"encoding/json"

	"weproxy/acc/libgo/logx"

	"weproxy/acc/app/acc-serv/internal/proto"
)

// TAG ...
const TAG = "[ss]"

// init ...
func init() {
	proto.Register("ss", New)
}

// New ...
func New(j json.RawMessage) (proto.Server, error) {
	return &server_t{}, nil
}

// server_t ...
type server_t struct {
}

// Start ...
func (m *server_t) Start() error {
	logx.D("%v Start()", TAG)
	return nil
}

// Close ...
func (m *server_t) Close() error {
	logx.D("%v Close()", TAG)
	return nil
}
