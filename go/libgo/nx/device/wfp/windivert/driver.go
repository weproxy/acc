//
// weproxy@foxmail.com 2022/10/20
//

package windivert

import (
	"archive/zip"
	"bytes"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"

	"golang.org/x/sys/windows"
	"golang.org/x/sys/windows/registry"
)

// //////////////////////////////////////////////////////////////////////////////////////////////////
var (
	_WinDivert_sys = ""
	_WinDivert_dll = ""
	_deviceName    = windows.StringToUTF16Ptr("WinDivert")
)

////////////////////////////////////////////////////////////////////////////////////////////////////

// // checkForWow64 ...
// func checkForWow64() error {
// 	var b bool
// 	err := windows.IsWow64Process(windows.CurrentProcess(), &b)
// 	if err != nil {
// 		return fmt.Errorf("Unable to determine whether the process is running under WOW64: %v", err)
// 	}
// 	if b {
// 		return fmt.Errorf("You must use the 64-bit version of WireGuard on this computer.")
// 	}
// 	return nil
// }

func init() {
	// if err := checkForWow64(); err != nil {
	// 	panic(err)
	// }

	system32, err := windows.GetSystemDirectory()
	if err != nil {
		panic(err)
	}

	_WinDivert_sys = filepath.Join(system32, "WinDivert"+strconv.Itoa(32<<(^uint(0)>>63))+".sys")
	_WinDivert_dll = filepath.Join(system32, "WinDivert.dll")

	err = installDriver()
	if err != nil {
		removeDriver()
		exec.Command("sc", "delete", "WinDivert").Run()

		err = installDriver()
		if err != nil {
			fmt.Printf("[windivert] installDriver() error: %v\n", err)
		}
	}
}

// installDriver ...
func installDriver() error {
	mutex, err := windows.CreateMutex(nil, false, windows.StringToUTF16Ptr("WinDivertDriverInstallMutex"))
	if err != nil {
		fmt.Printf("[windivert] windows.CreateMutex() error: %v\n", err)
		return err
	}
	defer func(mutex windows.Handle) {
		windows.ReleaseMutex(mutex)
		windows.CloseHandle(mutex)
	}(mutex)

	event, err := windows.WaitForSingleObject(mutex, windows.INFINITE)
	if err != nil {
		fmt.Printf("[windivert] windows.WaitForSingleObject() error: %v\n", err)
		return err
	}
	switch event {
	case windows.WAIT_OBJECT_0, windows.WAIT_ABANDONED:
	default:
		return errors.New("wait for object error")
	}

	manager, err := windows.OpenSCManager(nil, nil, windows.SC_MANAGER_ALL_ACCESS)
	if err != nil {
		fmt.Printf("[windivert] windows.OpenSCManager() error: %v\n", err)
		return err
	}
	defer windows.CloseServiceHandle(manager)

	service, err := windows.OpenService(manager, _deviceName, windows.SERVICE_ALL_ACCESS)
	if err == nil {
		windows.CloseServiceHandle(service)
		// fmt.Printf("[windivert] windows.OpenService() ok\n")
		return nil
	}

	sys, err := getDriverFileName()
	if err != nil {
		fmt.Printf("[windivert] getDriverFileName() error: %v\n", err)
		return err
	}

	// logx.D("[windivert] _WinDivert_sys = %s", _WinDivert_sys)
	// logx.D("[windivert] _WinDivert_dll = %s", _WinDivert_dll)

	service, err = windows.CreateService(manager, _deviceName, _deviceName,
		windows.SERVICE_ALL_ACCESS, windows.SERVICE_KERNEL_DRIVER, windows.SERVICE_DEMAND_START,
		windows.SERVICE_ERROR_NORMAL, windows.StringToUTF16Ptr(sys), nil, nil, nil, nil, nil)
	if err != nil {
		if err == windows.ERROR_SERVICE_EXISTS {
			return nil
		}

		fmt.Printf("[windivert] windows.CreateService() error: %v\n", err)

		service, err = windows.OpenService(manager, _deviceName, windows.SERVICE_ALL_ACCESS)
		if err != nil {
			fmt.Printf("[windivert] windows.OpenService() error: %v\n", err)
			return err
		}
	}
	defer windows.CloseServiceHandle(service)

	if err := windows.StartService(service, 0, nil); err != nil && err != windows.ERROR_SERVICE_ALREADY_RUNNING {
		fmt.Printf("[windivert] windows.StartService() error: %v\n", err)
		return err
	}

	if err := windows.DeleteService(service); err != nil && err != windows.ERROR_SERVICE_MARKED_FOR_DELETE {
		fmt.Printf("[windivert] windows.DeleteService() error: %v\n", err)
		return err
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// removeDriver ...
func removeDriver() error {
	status := windows.SERVICE_STATUS{}

	manager, err := windows.OpenSCManager(nil, nil, windows.SC_MANAGER_ALL_ACCESS)
	if err != nil {
		return err
	}
	defer windows.CloseServiceHandle(manager)

	service, err := windows.OpenService(manager, _deviceName, windows.SERVICE_ALL_ACCESS)
	if err != nil {
		if err == windows.ERROR_SERVICE_DOES_NOT_EXIST {
			return nil
		}

		return err
	}
	defer windows.CloseServiceHandle(service)

	if err := windows.ControlService(service, windows.SERVICE_CONTROL_STOP, &status); err != nil {
		if err == windows.ERROR_SERVICE_NOT_ACTIVE {
			return nil
		}

		return err
	}

	if err := windows.DeleteService(service); err != nil {
		if err == windows.ERROR_SERVICE_MARKED_FOR_DELETE {
			return nil
		}

		return err
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// getDriverFileName ...
func getDriverFileName() (string, error) {
	key, err := registry.OpenKey(registry.LOCAL_MACHINE,
		"System\\CurrentControlSet\\Services\\EventLog\\System\\WinDivert", registry.QUERY_VALUE)
	if err != nil {
		if _, err := os.Stat(_WinDivert_sys); err != nil {
			if err := downloadDriver(); err != nil {
				return "", fmt.Errorf("windivert error: %w", err)
			}
		}

		if err := registerEventSource(_WinDivert_sys); err != nil {
			return "", err
		}

		return _WinDivert_sys, nil
	}
	defer key.Close()

	val, _, err := key.GetStringValue("EventMessageFile")
	if err != nil {
		return "", err
	}

	if _, err := os.Stat(val); err != nil {
		if _, err := os.Stat(_WinDivert_sys); err != nil {
			if err := downloadDriver(); err != nil {
				return "", fmt.Errorf("windivert error: %w", err)
			}
		}

		if err := registerEventSource(_WinDivert_sys); err != nil {
			return "", err
		}

		return _WinDivert_sys, nil
	}

	return val, nil
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// registerEventSource ...
func registerEventSource(sys string) error {
	key, _, err := registry.CreateKey(registry.LOCAL_MACHINE,
		"System\\CurrentControlSet\\Services\\EventLog\\System\\WinDivert", registry.ALL_ACCESS)
	if err != nil {
		return err
	}
	defer key.Close()

	if err := key.SetStringValue("EventMessageFile", sys); err != nil {
		return err
	}

	const types = 7
	if err := key.SetDWordValue("TypesSupported", types); err != nil {
		return err
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// extractDriver ...
func extractDriver() (err error) {
	sys, dll := getEmbedDriverFileData()

	if _, err := os.Stat(_WinDivert_sys); err == nil {
		return nil
	}

	if err = ioutil.WriteFile(_WinDivert_sys, sys, 0444); err != nil {
		return err
	}

	if err = ioutil.WriteFile(_WinDivert_dll, dll, 0444); err != nil {
		return err
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// downloadDriver ...
func downloadDriver() (err error) {
	// LX @20220728
	if true {
		return extractDriver()
	}

	var (
		ver = "2.2.0"
		url = "https://github.com/basil00/Divert/releases/download/v" + ver + "/WinDivert-" + ver + "-A.zip"
		sys = ""
		dll = ""
	)

	if ^uint(0)>>63 == 1 {
		sys = "x64/WinDivert64.sys"
		dll = "x64/WinDivert.dll"
	} else {
		sys = "x86/WinDivert32.sys"
		dll = "x86/WinDivert.dll"
	}

	if _, err := os.Stat(_WinDivert_sys); err == nil {
		return nil
	}

	resp, err := http.Get(url)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("http status code is %v", resp.StatusCode)
	}

	zipFile, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return err
	}

	reader := bytes.NewReader(zipFile)

	zipReader, err := zip.NewReader(reader, int64(reader.Len()))
	if err != nil {
		return err
	}

	for _, file := range zipReader.File {
		if strings.HasSuffix(file.Name, sys) {
			f, err := file.Open()
			if err != nil {
				return err
			}
			defer f.Close()

			data, err := ioutil.ReadAll(f)
			if err != nil {
				return err
			}

			if err = ioutil.WriteFile(_WinDivert_sys, data, 0444); err != nil {
				return err
			}
		}

		if strings.HasSuffix(file.Name, dll) {
			f, err := file.Open()
			if err != nil {
				return err
			}
			defer f.Close()

			data, err := ioutil.ReadAll(f)
			if err != nil {
				return err
			}

			if err = ioutil.WriteFile(_WinDivert_dll, data, 0444); err != nil {
				return err
			}
		}
	}

	return nil
}
