//
// weproxy@foxmail.com 2022/10/29
//

package iptbl

import "weproxy/acc/libgo/logx"

// Detect ...
func Detect() (err error) {
	logx.D("%v Detect()", TAG)
	return
}

// detectTPROXY ...
func detectTPROXY() (bool, error) {
	return false, nil
}

// detectDNAT ...
func detectDNAT() (bool, error) {
	return false, nil
}

// detectTUN ...
func detectTUN() (bool, error) {
	return false, nil
}

// detectIPSET ...
func detectIPSET() (bool, error) {
	return false, nil
}

// detectRAW ...
func detectRAW() (bool, error) {
	return false, nil
}

// detectPCAP ...
func detectPCAP() (bool, error) {
	return false, nil
}
