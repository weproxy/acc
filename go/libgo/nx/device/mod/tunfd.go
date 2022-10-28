//
// weproxy@foxmail.com 2022/10/20
//

//go:build linux || darwin
// +build linux darwin

package mod

import (
	_ "weproxy/acc/libgo/nx/device/tunfd"
)
