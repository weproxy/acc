//
// weproxy@foxmail.com 2022/10/20
//

package rule

import (
	"math/rand"
	"net"
	"time"

	"weproxy/acc/libgo/logx"
)

const TAG = "[rule]"

////////////////////////////////////////////////////////////////////////////////

// Rule ...
type Rule struct {
	Servs []string
}

// Serv get one of Servs
func (m *Rule) Serv() string {
	if n := len(m.Servs); n > 0 {
		rand.Seed(time.Now().Unix())
		return m.Servs[rand.Intn(n)]
	}
	return ""
}

////////////////////////////////////////////////////////////////////////////////

// GetTCPRule
//
//	srcAddr = user's addr for load a rule config
//	one of dstHost and dstAddr maybe is nil
func GetTCPRule(srcAddr net.Addr, dstHost string, dstAddr net.Addr) (*Rule, error) {
	r := &Rule{
		// Servs: []string{"s5://127.0.0.1:19780"},
		Servs: []string{"direct"},
	}

	if len(dstHost) > 0 || IsHTTPsAddr(dstAddr) {
		logx.I("%s HTP %s -> %s, from %v", TAG, dstHost, r.Servs, srcAddr)
	} else {
		logx.I("%s TCP %v -> %s, from %v", TAG, dstAddr, r.Servs, srcAddr)
	}

	return r, nil
}

////////////////////////////////////////////////////////////////////////////////

// GetUDPRule
//
//	srcAddr = user's addr for load a rule config
//	one of dstHost and dstAddr maybe is nil
func GetUDPRule(srcAddr net.Addr, dstHost string, dstAddr net.Addr) (*Rule, error) {
	if len(dstHost) > 0 || IsDNSAddr(dstAddr) {
		return GetDNSRule(srcAddr, dstHost, dstAddr)
	}

	r := &Rule{
		// Servs: []string{"s5://127.0.0.1:19780"},
		Servs: []string{"direct"},
	}

	logx.I("%s UDP %v -> %s, from %v", TAG, dstAddr, r.Servs, srcAddr)

	return r, nil
}

////////////////////////////////////////////////////////////////////////////////

// GetDNSRule
//
//	srcAddr = user's addr for load a rule config
//	one of dstHost and dstAddr maybe is nil
func GetDNSRule(srcAddr net.Addr, dstHost string, dstAddr net.Addr) (*Rule, error) {
	r := &Rule{
		// Servs: []string{"dns://127.0.0.1:15353"},
		// Servs: []string{"dns://8.8.8.8:53"},
		Servs: []string{"dns://direct"},
		// Servs: []string{"dns://223.5.5.5:53"},
	}

	logx.I("%s DNS %s -> %s, from %v", TAG, dstHost, r.Servs, srcAddr)

	return r, nil
}

////////////////////////////////////////////////////////////////////////////////

// IsHTTPsAddr ...
func IsHTTPsAddr(addr net.Addr) bool {
	if addr != nil {
		switch addr := addr.(type) {
		case *net.TCPAddr:
			return IsHTTPsPort(addr.Port)
		case *net.UDPAddr:
		default:
		}
	}
	return false
}

// IsDNSAddr ...
func IsDNSAddr(addr net.Addr) bool {
	if addr != nil {
		switch addr := addr.(type) {
		case *net.TCPAddr:
		case *net.UDPAddr:
			return IsDNSPort(addr.Port)
		default:
		}
	}
	return false
}

// IsHTTPsPort ...
func IsHTTPsPort(port int) bool {
	return port == 80 || port == 443 /*|| port == 8080*/
}

// IsDNSPort ...
func IsDNSPort(port int) bool {
	return port == 53 /*|| port == 54*/
}
