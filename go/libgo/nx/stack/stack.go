//
// weproxy@foxmail.com 2022/10/20
//

package stack

import (
	"weproxy/acc/libgo/nx/stack/gvisor"
	"weproxy/acc/libgo/nx/stack/netstk"
)

////////////////////////////////////////////////////////////////////////////////

// New ...
func New() (netstk.Stack, error) {
	return gvisor.New()
}
