//
// weproxy@foxmail.com 2022/10/20
//

package core

import (
	"webproxy/acc/app/acc-serv/internal/proto"
)

// Main ...
func Main() {
	proto.Init(nil)

	// signal.Notify()

	proto.Deinit()
}
