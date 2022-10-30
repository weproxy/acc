//
// weproxy@foxmail.com 2022/10/20
//

package core

import (
	"os"

	"weproxy/acc/libgo/fx/signal"
	"weproxy/acc/libgo/logx"

	"weproxy/acc/app/acc-serv/internal/api"
	"weproxy/acc/app/acc-serv/internal/conf"
	"weproxy/acc/app/acc-serv/internal/proto"

	_ "weproxy/acc/app/acc-serv/internal/proto/dns"
	_ "weproxy/acc/app/acc-serv/internal/proto/htp"
	_ "weproxy/acc/app/acc-serv/internal/proto/kc"
	_ "weproxy/acc/app/acc-serv/internal/proto/qc"
	_ "weproxy/acc/app/acc-serv/internal/proto/s5"
	_ "weproxy/acc/app/acc-serv/internal/proto/ss"
)

// Main ...
func Main() {
	js, err := conf.ReadConfig()
	if err != nil {
		logx.E("[core] conf.ReadConfig(), err: %v", err)
		return
	}

	////////////////////////////////////////////////////////
	// proto init
	err = proto.Init(js.Servers)
	if err != nil {
		logx.E("[core] proto.Init(), err: %v", err)
		return
	}
	// proto deinit
	defer proto.Deinit()

	////////////////////////////////////////////////////////
	// api init
	err = api.Init()
	if err != nil {
		logx.E("[core] api.Init(), err: %v", err)
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
