package filter

import (
	"net"
	"sync"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/device/wfp/filter/iptree"
)

// IPFilter ...
type IPFilter struct {
	sync.RWMutex
	Tree  *iptree.Tree
	Rules map[string]bool
	Final bool
	// reader *maxminddb.Reader
}

// NewIPFilter ...
func NewIPFilter() *IPFilter {
	f := &IPFilter{
		RWMutex: sync.RWMutex{},
		Tree:    iptree.NewTree(),
	}
	return f
}

// IgnorePrivate ...
// ingore private address
func (f *IPFilter) IgnorePrivate() {
	for _, s := range []string{
		// RFC 1918: private IPv4 networks
		"10.0.0.0/8",
		"172.16.0.0/12",
		"192.168.0.0/16",
		// RFC 4193: IPv6 ULAs
		"fc00::/7",
		// RFC 6598: reserved prefix for CGNAT
		"100.64.0.0/10",
	} {
		_, ipNet, err := net.ParseCIDR(s)
		if err != nil {
			// log.Panic(err)
			logx.E("%v", err)
			return
		}
		f.Tree.InplaceInsertNet(ipNet, struct{}{})
	}
}

// Close ...
func (f *IPFilter) Close() error {
	// if f.reader != nil {
	// 	return f.reader.Close()
	// }
	return nil
}

// // SetGeoIP ...
// func (f *IPFilter) SetGeoIP(s string, proxy, bypass []string, final bool) (err error) {
// 	f.Lock()
// 	defer f.Unlock()

// 	f.reader, err = maxminddb.Open(s)
// 	if err != nil {
// 		return
// 	}

// 	f.Rules = make(map[string]bool)
// 	for _, v := range proxy {
// 		f.Rules[v] = true
// 	}
// 	for _, v := range bypass {
// 		f.Rules[v] = false
// 	}

// 	f.Final = final
// 	return
// }

// Add ...
func (f *IPFilter) Add(s string) error {
	f.Lock()
	err := f.UnsafeAdd(s)
	f.Unlock()
	return err
}

// UnsafeAdd ...
func (f *IPFilter) UnsafeAdd(s string) error {
	ip := net.ParseIP(s)
	if ip != nil {
		return f.addIP(ip)
	}

	_, ipNet, err := net.ParseCIDR(s)
	if err != nil {
		return err
	}

	return f.addCIDR(ipNet)
}

// addIP ...
func (f *IPFilter) addIP(ip net.IP) error {
	f.Tree.InplaceInsertIP(ip, nil)
	return nil
}

// addCIDR ...
func (f *IPFilter) addCIDR(ip *net.IPNet) error {
	f.Tree.InplaceInsertNet(ip, nil)
	return nil
}

// Lookup ...
func (f *IPFilter) Lookup(ip net.IP) bool {
	f.RLock()
	defer f.RUnlock()

	v, ok := f.Tree.GetByIP(ip)
	if ok {
		return v == nil
	}

	// // geology record
	// type Record struct {
	// 	Country struct {
	// 		ISOCode string `maxminddb:"iso_code"`
	// 	} `maxminddb:"country"`
	// }

	// if f.reader == nil {
	// 	return false
	// }

	// record := Record{}
	// if err := f.reader.Lookup(ip, &record); err != nil {
	// 	return f.Final
	// }

	// b, ok := f.Rules[record.Country.ISOCode]
	// if ok {
	// 	return b
	// }
	return f.Final
}
