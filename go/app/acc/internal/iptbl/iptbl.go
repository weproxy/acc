//
// weproxy@foxmail.com 2022/10/29
//

package iptbl

const TAG = "[iptbl]"

// Mode ...
type Mode int

const (
	ModeTPROXY Mode = iota
	ModeDNAT
	ModeTUN
	ModeRAW
	ModePCAP
)

var (
	TCPMode Mode = ModeTPROXY
	UDPMode Mode = ModeTPROXY
)
