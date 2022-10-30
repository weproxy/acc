//
// weproxy@foxmail.com 2022/10/29
//

package api

import (
	"io"

	"weproxy/acc/libgo/logx"
)

const TAG = "[api]"

////////////////////////////////////////////////////////////////////////////////

// _closers ...
var _closers []io.Closer

// Init ...
func Init() (err error) {
	logx.D("%v Init()", TAG)

	defer func() {
		if err != nil {
			closeAll()
		}
	}()

	////////////////////////////////////////////////////////
	// api server
	svr, err := newCtrlServ()
	if err != nil {
		return
	}
	_closers = append(_closers, svr)

	////////////////////////////////////////////////////////
	// conf client
	cfg, err := newConfCli()
	if err != nil {
		return
	}
	_closers = append(_closers, cfg)

	////////////////////////////////////////////////////////
	// turn client
	cli, err := newTurnCli()
	if err != nil {
		return
	}
	_closers = append(_closers, cli)

	return nil
}

// closeAll ...
func closeAll() {
	for i := len(_closers) - 1; i >= 0; i-- {
		_closers[i].Close()
	}
	_closers = nil
}

// Deinit ...
func Deinit() (err error) {
	closeAll()
	logx.D("%v Deinit()", TAG)
	return nil
}
