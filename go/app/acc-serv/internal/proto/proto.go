//
// weproxy@foxmail.com 2022/10/20
//

package proto

import (
	"encoding/json"

	"golang.org/x/net/dns/dnsmessage"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/dns"
)

// TAG ...
const TAG = "[proto]"

// _protos/_servers ...
var (
	_protos  = make(map[string]NewServerFn)
	_servers = make(map[string]Server)
)

// Server ...
type Server interface {
	Start() error
	Close() error
}

// NewServerFn ...
type NewServerFn func(j json.RawMessage) (Server, error)

// Register ...
func Register(proto string, fn NewServerFn) {
	logx.D("%v Register(%s)", TAG, proto)
	_protos[proto] = fn
}

////////////////////////////////////////////////////////////////////////////////

// _fakes
var _fakes = make(map[string] /*domain*/ []string /*ips*/)

// dnsSetFakeProvideFn ...
func dnsSetFakeProvideFn() error {
	// load fakes...
	err := func() error {
		// TODO...
		_fakes["fake.weproxy.test"] = []string{"1.2.3.4", "5.6.7.8"}
		return nil
	}()
	if err != nil {
		logx.E("%s load dns fakes, err: %v", TAG, err)
		return err
	}

	// SetFakeProvideFn ...
	dns.SetFakeProvideFn(func(domain string, typ dnsmessage.Type) []string {
		ss, ok := _fakes[domain]
		if ok {
			return ss
		}
		return nil
	})

	return nil
}

////////////////////////////////////////////////////////////////////////////////

// Init ..
func Init(servers []json.RawMessage) error {
	logx.D("%v Init()", TAG)

	// dns SetFakeProvideFn ...
	dnsSetFakeProvideFn()

	for _, j := range servers {
		var s struct {
			Proto    string `json:"proto,omitempty"`
			Disabled bool   `json:"disabled,omitempty"`
		}

		err := json.Unmarshal(j, &s)
		if err != nil || len(s.Proto) == 0 || s.Disabled {
			continue
		}

		if newFn, ok := _protos[s.Proto]; ok && newFn != nil {
			if svr, err := newFn(j); err != nil {
				return err
			} else if err = svr.Start(); err != nil {
				return err
			} else {
				_servers[s.Proto] = svr
			}
		}
	}

	return nil
}

// Deinit ...
func Deinit() {
	for _, svr := range _servers {
		svr.Close()
	}
	_servers = nil
	logx.D("%v Deinit()", TAG)
}
