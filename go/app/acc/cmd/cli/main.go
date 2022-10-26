//
// weproxy@foxmail.com 2022/10/20
//

package main

import (
	"time"

	"weproxy/acc/libgo/logx"

	"weproxy/acc/app/acc/internal/core"
)

func main() {
	logx.I("[main] ...")
	defer logx.I("[main] exit")

	// run core
	core.Main()

	// wait for log output
	time.Sleep(time.Second)
}
