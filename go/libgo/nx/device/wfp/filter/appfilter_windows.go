//go:build windows
// +build windows

package filter

import (
	"fmt"
	"path/filepath"
	"strings"
	"sync"
	"unsafe"

	"golang.org/x/sys/windows"
)

var (
	kernel32                   = windows.MustLoadDLL("kernel32.dll")
	queryFullProcessImageNameW = kernel32.MustFindProc("QueryFullProcessImageNameW")
)

// QueryFullProcessImageName is ...
func QueryFullProcessImageName(process windows.Handle, flags uint32, b []uint16) (string, error) {
	n := uint32(windows.MAX_PATH)

	// BOOL QueryFullProcessImageNameW(
	//   HANDLE hProcess,
	//   DWORD  dwFlags,
	//   LPSTR  lpExeName,
	//   PDWORD lpdwSize
	// );
	// https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-queryfullprocessimagenamew
	ret, _, errno := queryFullProcessImageNameW.Call(
		uintptr(process),
		uintptr(flags),
		uintptr(unsafe.Pointer(&b[0])),
		uintptr(unsafe.Pointer(&n)),
	)
	if ret == 0 {
		return "", errno
	}
	return windows.UTF16ToString(b[:n]), nil
}

// QueryNameByPID is ...
func QueryNameByPID(id uint32, b []uint16) (string, error) {
	h, err := windows.OpenProcess(windows.PROCESS_QUERY_LIMITED_INFORMATION, false, id)
	if err != nil {
		return "", fmt.Errorf("open process error: %w", err)
	}
	defer windows.CloseHandle(h)

	path, err := QueryFullProcessImageName(h, 0, b)
	if err != nil {
		return "", fmt.Errorf("query full process name error: %w", err)
	}

	_, file := filepath.Split(path)
	return file, nil
}

// AppFilter is ...
type AppFilter struct {
	sync.RWMutex
	PIDs      map[uint32]struct{}
	Apps      map[string]struct{}
	buff      []uint16
	startOnce sync.Once
}

// NewAppFilter is ...
func NewAppFilter() *AppFilter {
	m := &AppFilter{
		PIDs: make(map[uint32]struct{}),
		Apps: make(map[string]struct{}),
		buff: make([]uint16, windows.MAX_PATH),
	}
	return m
}

// SetPIDs is ...
func (m *AppFilter) SetPIDs(ids []uint32) {
	m.Lock()
	for _, v := range ids {
		m.PIDs[v] = struct{}{}
	}
	m.Unlock()
}

// Add 如 Add("msedge.exe")
func (m *AppFilter) Add(s string) {
	m.Lock()
	m.UnsafeAdd(s)
	m.Unlock()
}

// UnsafeAdd 如 UnsafeAdd("msedge.exe")
func (m *AppFilter) UnsafeAdd(s string) {
	s = strings.TrimSpace(strings.ToLower(s))
	m.Apps[s] = struct{}{}
}

// Lookup is ...
func (m *AppFilter) Lookup(id uint32) bool {
	m.RLock()
	defer m.RUnlock()

	// use PID
	if _, ok := m.PIDs[id]; ok {
		return true
	}

	// use name
	file, err := QueryNameByPID(id, m.buff)
	if err != nil {
		return false
	}

	_, ok := m.Apps[strings.ToLower(file)]

	return ok
}

// // loopFix 消掉自动加的/已不存在的 pid
// func (m *AppFilter) loopFix() {
// 	fix := func() {
// 		m.RLock()
// 		defer m.RUnlock()

// 		for id, flg := range m.PIDs {
// 			if flg == _PID_ADDED {
// 				continue // 不消除主动加的那些
// 			}

// 			file, err := QueryNameByPID(id, m.buff)
// 			if err != nil {
// 				delete(m.PIDs, id)
// 				continue
// 			}

// 			s := strings.ToLower(file)
// 			if _, ok := m.Apps[s]; !ok {
// 				// 这个 pid 可能已经是新进程，名字已不是我们加的那个了
// 				delete(m.PIDs, id)
// 			}
// 		}
// 	}

// 	t := time.NewTicker(time.Second * 20)
// 	for range t.C {
// 		fix() // 消掉自动加的/已不存在的 pid
// 	}
// }
