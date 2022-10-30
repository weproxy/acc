//
// weproxy@foxmail.com 2022/10/29
//

package api

import (
	"io"

	"weproxy/acc/libgo/logx"
)

// newCtrlServ ...
func newCtrlServ() (io.Closer, error) {
	m := &ctrlServ{}
	if err := m.Start(); err != nil {
		return nil, err
	}
	return m, nil
}

// ctrlServ ...
type ctrlServ struct {
}

// Start ...
func (m *ctrlServ) Start() error {
	logx.D("%s ctrlServ.Start()", TAG)
	return nil
}

// Close ...
func (m *ctrlServ) Close() error {
	logx.D("%s ctrlServ.Close()", TAG)
	return nil
}
