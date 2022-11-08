//
// weproxy@foxmail.com 2022/10/20
//

package service

import (
	"fmt"
	"runtime"

	"github.com/spf13/cobra"
	"github.com/takama/daemon"
)

// AddTo ...
func AddTo(rootCmd *cobra.Command, appName, appDesc string) {
	var subCmds = map[string]struct {
		Run   func(d daemon.Daemon) (string, error)
		Short string
	}{
		"install": {func(d daemon.Daemon) (string, error) { return d.Install() }, "install servie"},
		"remove":  {func(d daemon.Daemon) (string, error) { return d.Remove() }, "remove service"},
		"start":   {func(d daemon.Daemon) (string, error) { return d.Start() }, "start service"},
		"stop":    {func(d daemon.Daemon) (string, error) { return d.Stop() }, "stop service"},
		"status":  {func(d daemon.Daemon) (string, error) { return d.Status() }, "get service status"},
	}

	for use, it := range subCmds {
		rootCmd.AddCommand(&cobra.Command{
			Use:   use,
			Short: it.Short,
			Run: func(cmd *cobra.Command, args []string) {
				if len(cmd.Short) > 0 {
					fmt.Println(cmd.Short, "...")
				} else {
					fmt.Println(cmd.Use, "...")
				}

				if it, ok := subCmds[cmd.Use]; ok && it.Run != nil {
					typ := daemon.SystemDaemon
					if runtime.GOOS == "darwin" {
						typ = daemon.GlobalDaemon
					}

					d, err := daemon.New(appName, appDesc, typ)
					if err == nil && d != nil {
						if ret, err := it.Run(d); err != nil {
							fmt.Println("error:", err)
						} else if len(ret) > 0 {
							fmt.Println(ret)
						} else {
							fmt.Println("done")
						}
					} else {
						fmt.Println("error:", err)
					}
				} else {
					fmt.Println("error: invalid cmd")
				}
			},
		})
	}
}

// New ...
func New(appName, appDesc string) *cobra.Command {
	var svcCmd = &cobra.Command{
		Use:   appName,
		Short: appName + " install|remove|start|stop|status",
	}
	svcCmd.Run = func(cmd *cobra.Command, args []string) {
		svcCmd.Help()
	}

	AddTo(svcCmd, appName, appDesc)

	return svcCmd
}
