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

// QueryFullProcessImageName ...
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

// QueryNameByPID ...
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

// AppFilter ...
type AppFilter struct {
	sync.RWMutex
	PIDs      map[uint32]struct{}
	Apps      map[string]struct{}
	buff      []uint16
	startOnce sync.Once
}

// NewAppFilter ...
func NewAppFilter() *AppFilter {
	m := &AppFilter{
		PIDs: make(map[uint32]struct{}),
		Apps: make(map[string]struct{}),
		buff: make([]uint16, windows.MAX_PATH),
	}
	return m
}

// SetPIDs ...
func (m *AppFilter) SetPIDs(ids []uint32) {
	m.Lock()
	for _, v := range ids {
		m.PIDs[v] = struct{}{}
	}
	m.Unlock()
}

// Add -> exeFileName e.g. "msedge.exe"
func (m *AppFilter) Add(exeFileName string) {
	m.Lock()
	m.UnsafeAdd(exeFileName)
	m.Unlock()
}

// UnsafeAdd -> exeFileName e.g. "msedge.exe"
func (m *AppFilter) UnsafeAdd(exeFileName string) {
	exeFileName = strings.TrimSpace(strings.ToLower(exeFileName))
	m.Apps[exeFileName] = struct{}{}
}

// Lookup ...
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
