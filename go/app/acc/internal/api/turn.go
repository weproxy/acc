//
// weproxy@foxmail.com 2022/10/29
//

package api

import (
	"io"

	"weproxy/acc/libgo/logx"
)

// newTurnCli ...
func newTurnCli() (io.Closer, error) {
	m := &turnCli{}
	if err := m.Start(); err != nil {
		return nil, err
	}
	return m, nil
}

// turnCli ...
type turnCli struct {
}

// Start ...
func (m *turnCli) Start() error {
	logx.D("%s turnCli.Start()", TAG)
	return nil
}

// Close ...
func (m *turnCli) Close() error {
	logx.D("%s turnCli.Close()", TAG)
	return nil
}
