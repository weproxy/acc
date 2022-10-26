//
// weproxy@foxmail.com 2022/10/20
//

package core

import (
	"os"

	"weproxy/acc/libgo/fx/signal"
	"weproxy/acc/libgo/logx"

	"weproxy/acc/app/acc/internal/proto"
)

func Main() {
	// proto init
	err := proto.Init()
	// proto deinit
	defer proto.Deinit()
	if err != nil {
		logx.E("[core] proto::Init(), err: %v", err)
		return
	}

	// Wait for Ctrl+C or kill -x
	signal.WaitNotify(func() {
		logx.W("[signal] quit")
	}, os.Interrupt, os.Kill)
}
