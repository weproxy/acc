//
// weproxy@foxmail.com 2022/10/20
//

package core

import (
	"os"
	"runtime"

	"weproxy/acc/libgo/fx/signal"
	"weproxy/acc/libgo/logx"

	"weproxy/acc/app/acc/internal/api"
	"weproxy/acc/app/acc/internal/board"
	"weproxy/acc/app/acc/internal/iptbl"
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

// Main ...
func Main() {
	////////////////////////////////////////////////////////
	// detect
	if runtime.GOOS == "linux" {
		board.Detect()
		iptbl.Detect()
	}

	////////////////////////////////////////////////////////
	// proto init
	err := proto.Init()
	if err != nil {
		logx.E("[core] proto::Init(), err: %v", err)
		return
	}
	// proto deinit
	defer proto.Deinit()

	////////////////////////////////////////////////////////
	// api init
	err = api.Init()
	if err != nil {
		logx.E("[core] api::Init(), err: %v", err)
		return
	}
	// api deinit
	defer api.Deinit()

	////////////////////////////////////////////////////////
	// Wait for Ctrl+C or kill -x
	signal.WaitNotify(func() {
		logx.W("[signal] quit")
	}, os.Interrupt, os.Kill)
}
