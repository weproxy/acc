//
// weproxy@foxmail.com 2022/10/20
//

package proto

import (
	"io"
	"net/url"
	"runtime"
	"strings"

	"golang.org/x/net/dns/dnsmessage"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/device"
	"weproxy/acc/libgo/nx/dns"
	"weproxy/acc/libgo/nx/server/dnat"
	"weproxy/acc/libgo/nx/server/tproxy"
	"weproxy/acc/libgo/nx/sni"
	"weproxy/acc/libgo/nx/stack"
	"weproxy/acc/libgo/nx/stack/netstk"

	"weproxy/acc/app/acc/internal/proto/rule"
)

const TAG = "proto"

////////////////////////////////////////////////////////////////////////////////

// _protos ...
var _protos = make(map[string]NewHandlerFn)

// Handler ...
type Handler interface {
	io.Closer
	Handle(c netstk.Conn, head []byte)
	HandlePacket(pc netstk.PacketConn, head []byte)
}

// NewHandlerFn ...
type NewHandlerFn func(serv string) (Handler, error)

// Register ...
func Register(protos []string, fn NewHandlerFn) {
	logx.D("%v Register(%s)", TAG, protos)
	for _, proto := range protos {
		_protos[proto] = fn
	}
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

// _closers ...
var _closers []io.Closer

// Init ...
func Init() (err error) {
	logx.D("%v Init()", TAG)

	// dns SetFakeProvideFn ...
	dnsSetFakeProvideFn()

	defer func() {
		if err != nil {
			closeAll()
		}
	}()

	// net stack
	stk, err := stack.New()
	if err != nil {
		return err
	}
	_closers = append(_closers, stk)

	// net device
	dev, err := newDevice()
	if err != nil {
		return err
	}
	_closers = append(_closers, dev)

	// stack handler
	h := &stackHandler{}

	// start stack
	err = stk.Start(h, dev, 1500)
	if err != nil {
		return err
	}

	// tproxy/dnat server
	if runtime.GOOS == "linux" {
		var svr []io.Closer
		svr, err = newServer(h)
		if err != nil {
			return
		}
		_closers = append(_closers, svr...)
	}

	return nil
}

// newDevice ...
func newDevice() (dev netstk.Device, err error) {
	if runtime.GOOS == "ios" || runtime.GOOS == "android" {
		// tunfd
		tunfd := 0 // TODO read from setting
		cfg := device.Conf{
			"tunfd": tunfd,
		}
		return device.New(device.TypeTUNFD, cfg)
	}

	if runtime.GOOS == "windows" {
		// wfp
		cfg := device.Conf{}
		return device.New(device.TypeWFP, cfg)
	}

	if true {
		// tun
		cfg := device.Conf{
			"ifname": "en0",
			"cidr":   "10.66.66.1/24",
		}
		return device.New(device.TypeTUN, cfg)
	}

	if true && runtime.GOOS == "linux" {
		// raw
		cfg := device.Conf{
			"ifname": "en0",
		}
		return device.New(device.TypeRAW, cfg)
	}

	// pacp
	cfg := device.Conf{
		"ifname": "en0",
		"cidr":   "10.6.6.1/24",
	}
	return device.New(device.TypePCAP, cfg)
}

// newServer ...
func newServer(h netstk.Handler) (arr []io.Closer, err error) {
	var svr io.Closer

	// tproxy server
	svr, err = tproxy.New(h, "10.20.30.40:5000")
	if err != nil {
		return
	}
	arr = append(arr, svr)

	// dnat server
	svr, err = dnat.New(h, "127.0.0.01:5050")
	if err != nil {
		return
	}
	arr = append(arr, svr)

	return
}

// closeAll ...
func closeAll() {
	for i := len(_closers) - 1; i >= 0; i-- {
		_closers[i].Close()
	}
	_closers = nil
}

// Deinit ...
func Deinit() error {
	closeAll()
	logx.D("%v Deinit()", TAG)
	return nil
}

////////////////////////////////////////////////////////////////////////////////

// stackHandler implements netstk.stackHandler
type stackHandler struct {
}

// Close ...
func (m *stackHandler) Close() error {
	return nil
}

// Handle TCP ...
func (m *stackHandler) Handle(c netstk.Conn) {
	caddr, raddr := c.LocalAddr(), c.RemoteAddr()
	// logx.D("%s Handle() %v->%v", TAG, caddr, raddr)
	defer c.Close()

	var head []byte
	var dstHost string
	if rule.IsHTTPsAddr(raddr) {
		done := false
		if dstHost, head, done = parseHTTPs(c); done {
			return
		}
	}

	// get rule
	proto, serv := func() (string, string) {
		serv, err := rule.GetTCPRule(caddr, dstHost, raddr)
		if err == nil {
			if !strings.Contains(serv, "://") {
				return serv, serv
			} else if uri, err := url.Parse(serv); err == nil {
				return uri.Scheme, serv
			}
		}
		return "", ""
	}()

	// get handler
	if fn, ok := _protos[proto]; ok && fn != nil {
		if h, err := fn(serv); err == nil && h != nil {
			h.Handle(c, head)
			return
		}
	}

	logx.W("%s TCP not found handler for: %s", TAG, serv)
}

// HandlePacket UDP ...
func (m *stackHandler) HandlePacket(pc netstk.PacketConn) {
	caddr, raddr := pc.LocalAddr(), pc.RemoteAddr()
	// logx.D("%s HandlePacket() %v->%v", TAG, caddr, raddr)
	defer pc.Close()

	var head []byte
	var dstHost string
	if rule.IsDNSAddr(raddr) {
		done := false
		if dstHost, head, done = parseDNS(pc); done {
			return
		}
	}

	// get rule
	proto, serv := func() (string, string) {
		serv, err := rule.GetUDPRule(caddr, dstHost, raddr)
		if err == nil {
			if !strings.Contains(serv, "://") {
				return serv, serv
			} else if uri, err := url.Parse(serv); err == nil {
				return uri.Scheme, serv
			}
		}
		return "", ""
	}()

	// get handler
	if fn, ok := _protos[proto]; ok && fn != nil {
		if h, err := fn(serv); err == nil && h != nil {
			h.HandlePacket(pc, head)
			return
		}
	}

	logx.W("%s UDP not found handler for: %s", TAG, serv)
}

////////////////////////////////////////////////////////////////////////////////

// parseHTTPs ...
func parseHTTPs(c netstk.Conn) (dstHost string, head []byte, done bool) {
	buf := make([]byte, 1024*8)

	n, err := c.Read(buf)
	if err != nil {
		done = true
		return
	} else if n <= 0 {
		return
	}

	head = buf[:n]
	dstHost, _, _ = sni.GetServerName(head)

	return
}

// parseDNS ...
func parseDNS(pc netstk.PacketConn) (dstHost string, head []byte, done bool) {
	buf := make([]byte, 1024*8)

	n, raddr, err := pc.ReadFrom(buf)
	if err != nil {
		done = true
		return
	} else if n <= 0 {
		return
	}

	head = buf[:n]

	// dns cache query
	msg, ans, _ := dns.OnRequest(head)
	if ans != nil && ans.Msg != nil {
		// dns cache answer
		if b := ans.Bytes(); len(b) > 0 {
			done = true
			pc.WriteTo(b, raddr)
			return
		}
	}

	if msg != nil && len(msg.Questions) > 0 {
		dstHost = strings.TrimSuffix(msg.Questions[0].Name.String(), ".")
	}

	return
}
