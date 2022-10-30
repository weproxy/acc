//
// weproxy@foxmail.com 2022/10/29
//

package logx

import (
	"fmt"
	"os"
	"syscall"

	"golang.org/x/sys/windows"
)

var (
	kernel32       = windows.MustLoadDLL("kernel32.dll")
	setConsoleMode = kernel32.MustFindProc("SetConsoleMode")
)

func init() {
	setConsoleColorMode()
}

// _setConsoleMode ...
func _setConsoleMode(fd syscall.Handle, mode uint32) (err error) {
	r, _, err := setConsoleMode.Call(uintptr(fd), uintptr(mode))
	if r == 0 {
		return err
	}
	return nil
}

// setConsoleColorMode ...
func setConsoleColorMode() (err error) {
	stdout := syscall.Handle(os.Stdout.Fd())

	var oldMode uint32
	err = syscall.GetConsoleMode(stdout, &oldMode)
	if err != nil {
		fmt.Println("GetConsoleMode err:", err)
		return
	}

	const ENABLE_VIRTUAL_TERMINAL_PROCESSING = uint32(0x0004)

	var newMode uint32 = oldMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING
	err = _setConsoleMode(stdout, newMode)
	if err != nil {
		fmt.Println("SetConsoleMode err:", err)
		return
	}

	// fmt.Println("SetConsoleMode: ", newMode)

	// runtime.SetFinalizer(f.file, (*file).close)

	return
}
