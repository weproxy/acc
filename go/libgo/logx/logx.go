//
// weproxy@foxmail.com 2022/10/20
//

package logx

import (
	"context"
	"fmt"
	"runtime"
	"strings"
	"time"
)

////////////////////////////////////////////////////////////////////////////////

// Level ...
type Level int

const (
	LeveNone Level = 0
	LeveErr  Level = 1
	LeveWrn  Level = 2
	LeveMsg  Level = 3
	LeveDbg  Level = 4
	LeveMax  Level = 50
)

// Color ...
type Color string

const (
	ColorClean   Color = "\033[0m"
	ColorWeak    Color = "\033[2m"
	ColorBlack   Color = "\033[30m"
	ColorRed     Color = "\033[31m"
	ColorGreen   Color = "\033[32m"
	ColorYellow  Color = "\033[33m"
	ColorBlue    Color = "\033[34m"
	ColorMagenta Color = "\033[35m"
	ColorCyan    Color = "\033[36m"
	ColorWhite   Color = "\033[37m"
)

////////////////////////////////////////////////////////////////////////////////

// conf_t ...
type conf_t struct {
	StdoutLevel  Level
	ReportLevel  Level
	ShowCaller   bool
	ShowDatetime bool
	WithColor    bool
}

// reach ...
func (m *conf_t) reach(lvl Level) bool {
	return m.StdoutLevel >= lvl
}

////////////////////////////////////////////////////////////////////////////////

var (
	_fileStart  int
	_stackStart int
)

// getCallerStack ...
func getCallerStack(start int) string {
	for i := start; i < start+10; i++ {
		pc, file, line, ok := runtime.Caller(i)
		if !ok {
			break
		}

		if idx := strings.Index(file, "/libgo/logx/"); idx >= 0 {
			if _fileStart == 0 {
				_fileStart = idx + 1
			}
			continue
		}

		if _stackStart == 0 {
			_stackStart = i
		}

		if idx := strings.Index(file, "/libgo/nx/stats"); idx >= 0 {
			pc, file, line, ok = runtime.Caller(i + 1)
		} else if idx := strings.Index(file, "/libgo/biz/report"); idx >= 0 {
			pc, file, line, ok = runtime.Caller(i + 4)
		}
		if !ok {
			return ""
		}

		if _fileStart >= 0 && len(file) > _fileStart {
			file = file[_fileStart:]
		}

		_ = pc

		return fmt.Sprintf("%v:%v ", file, line)

		// // 取函数名
		// var name string
		// if fn := runtime.FuncForPC(pc); nil != fn {
		// 	name = fn.Name()
		// 	if idx := strings.LastIndex(name, "."); idx >= 0 {
		// 		name = name[idx+1:]
		// 	}
		// }

		// return fmt.Sprintf("%v:%v @%v(): ", file, line, name)
	}

	return ""
}

////////////////////////////////////////////////////////////////////////////////

var (
	_cfg     = &conf_t{}
	_ctx     = context.TODO()
	_logCh   = make(chan logmsg, 1024*2)
	_Beijing = time.FixedZone("BeiJing time", int((8 * time.Hour).Seconds()))
)

// timeStr 输出当前时间字符串
func timeStr() string {
	return Now().Format("01-02 15:04:05.999999")
}

// Now ...
func Now() time.Time {
	return time.Now().In(_Beijing)
}

// /////////////////////////////////////////////////////////////////////////////

// logmsg ...
type logmsg struct {
	lvl Level
	msg string
}

// logf ...
func logf(lvl Level, tag string, color Color, format string, args ...interface{}) {
	var s string

	if _cfg.WithColor {
		s += string(ColorClean) + string(ColorWeak)
	}
	if _cfg.ShowDatetime {
		s += timeStr()
	}
	s += tag
	if _cfg.ShowCaller {
		s += getCallerStack(_stackStart)
	}
	if _cfg.WithColor {
		s += string(ColorClean) + string(color)
	}
	s += fmt.Sprintf(format, args...)
	if _cfg.WithColor {
		s += string(ColorClean)
	}
	s += "\n"

	// write to chan
	_logCh <- logmsg{lvl: lvl, msg: s}
}

////////////////////////////////////////////////////////////////////////////////

// init ...
func init() {
	_cfg.StdoutLevel = LeveDbg
	_cfg.ReportLevel = LeveDbg
	_cfg.ShowCaller = true
	_cfg.ShowDatetime = true
	_cfg.WithColor = true

	// read chan looping
	go func() {
		for {
			select {
			case <-_ctx.Done():
				return
			case log, ok := <-_logCh:
				if !ok {
					return
				}

				if _cfg.StdoutLevel >= log.lvl {
					fmt.Print(log.msg)
				}

				// if _cfg.ReportLevel >= log.lvl {
				// 	// TODO...
				// }
			}
		}
	}()
}

///////////////////////////////////////////////////////////////////////////////

// V http post
func V(format string, args ...interface{}) {
	logf(LeveMax, "[V]", ColorMagenta, format, args...)
}

// D debug
func D(format string, args ...interface{}) {
	if _cfg.reach(LeveDbg) {
		logf(LeveDbg, "[D]", ColorClean, format, args...)
	}
}

// I info
func I(format string, args ...interface{}) {
	if _cfg.reach(LeveMsg) {
		logf(LeveMsg, "[I]", ColorGreen, format, args...)
	}
}

// W warn
func W(format string, args ...interface{}) {
	if _cfg.reach(LeveWrn) {
		logf(LeveWrn, "[W]", ColorYellow, format, args...)
	}
}

// E error
func E(format string, args ...interface{}) {
	if _cfg.reach(LeveErr) {
		logf(LeveErr, "[E]", ColorRed, format, args...)
	}
}

// P any
func P(format string, args ...interface{}) {
	logf(LeveErr, "[P]", ColorCyan, format, args...)
}
