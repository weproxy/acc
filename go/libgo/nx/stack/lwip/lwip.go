//
// weproxy@foxmail.com 2022/10/20
//

package lwip

import (
	"errors"

	"weproxy/acc/libgo/nx/stack/netstk"
)

////////////////////////////////////////////////////////////////////////////////

// Stack impl interface netstk.Stack
type Stack struct {
	h   netstk.Handler
	dev netstk.Device
}

// Start ...
func (m *Stack) Start(h netstk.Handler, dev netstk.Device, mtu int) (err error) {
	m.h, m.dev = h, dev
	return nil
}

// Close ...
func (m *Stack) Close() error {
	return nil
}

////////////////////////////////////////////////////////////////////////////////

// NewStack ...
func NewStack() (netstk.Stack, error) {
	return &Stack{}, errors.New("not impl")
}
