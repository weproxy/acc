//
// weproxy@foxmail.com 2022/10/20
//

package core

import (
	"os"

	"weproxy/acc/libgo/fx/signal"
	"weproxy/acc/libgo/logx"

	"weproxy/acc/app/acc/internal/proto"

	_ "weproxy/acc/app/acc/internal/proto/direct"
	_ "weproxy/acc/app/acc/internal/proto/gaap"
	_ "weproxy/acc/app/acc/internal/proto/htp"
	_ "weproxy/acc/app/acc/internal/proto/kc"
	_ "weproxy/acc/app/acc/internal/proto/qc"
	_ "weproxy/acc/app/acc/internal/proto/s5"
	_ "weproxy/acc/app/acc/internal/proto/ss"
	_ "weproxy/acc/libgo/nx/device/mod"
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
