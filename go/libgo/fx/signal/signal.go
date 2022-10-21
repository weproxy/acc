//
// weproxy@foxmail.com 2022/10/20
//

package signal

import (
	"os"
	"os/signal"
)

// WaitNotify ...
func WaitNotify(cb func(), sigs ...os.Signal) {
	sig := make(chan os.Signal, 1)

	signal.Notify(sig, sigs...)

	_, ok := <-sig
	if ok && cb != nil {
		cb()
	}
}
