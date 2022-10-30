//
// weproxy@foxmail.com 2022/10/29
//

package api

import (
	"io"

	"weproxy/acc/libgo/logx"
)

// newConfCli ...
func newConfCli() (io.Closer, error) {
	m := &confCli{}
	if err := m.Start(); err != nil {
		return nil, err
	}
	return m, nil
}

// confCli ...
type confCli struct {
}

// Start ...
func (m *confCli) Start() error {
	logx.D("%s confCli.Start()", TAG)
	return nil
}

// Close ...
func (m *confCli) Close() error {
	logx.D("%s confCli.Close()", TAG)
	return nil
}
