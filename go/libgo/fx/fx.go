//
// weproxy@foxmail.com 2022/10/20
//

package fx

import "fmt"

// FormatBytes ...
func FormatBytes(n int64) string {
	if n < 1024 {
		return fmt.Sprintf("%vB", n)
	} else if n < 1024*1024 {
		return fmt.Sprintf("%.02fKB", float64(n)/1024.0)
	} else if n < 1024*1024*1024 {
		return fmt.Sprintf("%.02fMB", float64(n)/1024.0/1024.0)
	} else if n < 1024*1024*1024*1024 {
		return fmt.Sprintf("%.02fGB", float64(n)/1024.0/1024.0/1024.0)
	} else if n < 1024*1024*1024*1024*1024 {
		return fmt.Sprintf("%.02fPB", float64(n)/1024.0/1024.0/1024.0/1024.0)
	} else if n < 1024*1024*1024*1024*1024*1024 {
		return fmt.Sprintf("%.02fTB", float64(n)/1024.0/1024.0/1024.0/1024.0/1024.0)
	}

	return fmt.Sprintf("%vB", n)
}
