//
// weproxy@foxmail.com 2022/10/20
//

package rule

import (
	"net"

	"weproxy/acc/app/acc/internal/conf"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx"
)

const TAG = "[rule]"

////////////////////////////////////////////////////////////////////////////////

// GetTCPRule
//
//	srcAddr = user's addr for load a rule config
//	one of dstHost and dstAddr maybe is nil
func GetTCPRule(srcAddr net.Addr, dstHost string, dstAddr net.Addr) (serv string, err error) {
	serv = func() string {
		c := conf.MustGet(nx.GetIP(srcAddr).String())

		var r *conf.Rule
		if len(dstHost) > 0 {
			r, _ = c.GetRule(dstHost)
		} else if dstAddr != nil {
			r, _ = c.GetRule(nx.GetIP(dstAddr).String())
		}
		if r != nil {
			return r.GetServ()
		}
		return c.GetMain()
	}()

	if len(dstHost) > 0 || IsHTTPsAddr(dstAddr) {
		logx.I("%s HTP %s -> %s, from %v", TAG, dstHost, serv, srcAddr)
	} else {
		logx.I("%s TCP %v -> %s, from %v", TAG, dstAddr, serv, srcAddr)
	}

	return serv, nil
}

////////////////////////////////////////////////////////////////////////////////

// GetUDPRule
//
//	srcAddr = user's addr for load a rule config
//	one of dstHost and dstAddr maybe is nil
func GetUDPRule(srcAddr net.Addr, dstHost string, dstAddr net.Addr) (serv string, err error) {
	if len(dstHost) > 0 || IsDNSAddr(dstAddr) {
		return GetDNSRule(srcAddr, dstHost, dstAddr)
	}

	serv = func() string {
		c := conf.MustGet(nx.GetIP(srcAddr).String())

		var r *conf.Rule
		if len(dstHost) > 0 {
			r, _ = c.GetRule(dstHost)
		} else if dstAddr != nil {
			r, _ = c.GetRule(nx.GetIP(dstAddr).String())
		}
		if r != nil {
			return r.GetServ()
		}
		return c.GetMain()
	}()

	logx.I("%s UDP %v -> %s, from %v", TAG, dstAddr, serv, srcAddr)

	return serv, nil
}

////////////////////////////////////////////////////////////////////////////////

// GetDNSRule
//
//	srcAddr = user's addr for load a rule config
//	one of dstHost and dstAddr maybe is nil
func GetDNSRule(srcAddr net.Addr, dstHost string, dstAddr net.Addr) (serv string, err error) {
	serv = func() string {
		c := conf.MustGet(nx.GetIP(srcAddr).String())

		var r *conf.Rule
		if len(dstHost) > 0 {
			r, _ = c.GetDNSRule(dstHost)
		} else if dstAddr != nil {
			r, _ = c.GetDNSRule(nx.GetIP(dstAddr).String())
		}
		if r != nil {
			return r.GetServ()
		}
		return c.GetMain()
	}()

	logx.I("%s DNS %s -> %s, from %v", TAG, dstHost, serv, srcAddr)

	return serv, nil
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
