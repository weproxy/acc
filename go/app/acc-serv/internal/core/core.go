//
// weproxy@foxmail.com 2022/10/20
//

package core

import (
	"os"

	"weproxy/acc/libgo/fx/signal"
	"weproxy/acc/libgo/logx"

	"weproxy/acc/app/acc-serv/internal/conf"
	"weproxy/acc/app/acc-serv/internal/proto"
)

// Main ...
func Main() {
	js, err := conf.ReadConfig()
	if err != nil {
		logx.E("[core] conf::ReadConfig(), err: %v", err)
		return
	}

	// proto init
	err = proto.Init(js.Servers)
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
