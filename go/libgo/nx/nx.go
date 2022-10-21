//
// weproxy@foxmail.com 2022/10/20
//

package nx

import "sync/atomic"

var (
	_id uint64
)

// NewID ...
func NewID() uint64 {
	return atomic.AddUint64(&_id, 1)
}
