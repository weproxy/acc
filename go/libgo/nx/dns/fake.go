//
// weproxy@foxmail.com 2022/10/22
//

package dns

import (
	"errors"
	"net"
	"strings"

	"golang.org/x/net/dns/dnsmessage"
)

////////////////////////////////////////////////////////////////////////////////

var (
	_FakeProvideFn FakeProvideFn
)

// SetFakeProvideFn ...
func SetFakeProvideFn(fn FakeProvideFn) {
	_FakeProvideFn = fn
}

// FakeProvideFn ...
type FakeProvideFn func(domain string, typ dnsmessage.Type) (ips []string)

// MakeFakeAnswer ...
// if provideFn is nil, use global fn set by SetFakeProvideFn()
func MakeFakeAnswer(b []byte, provideFn FakeProvideFn) (*Answer, error) {
	var msg dnsmessage.Message
	if err := msg.Unpack(b); err != nil {
		return nil, err
	}
	return MakeFakeAnswerMsg(&msg, provideFn)
}

// MakeFakeAnswerMsg ...
// if provideFn is nil, use global fn set by SetFakeProvideFn()
func MakeFakeAnswerMsg(msg *dnsmessage.Message, provideFn FakeProvideFn) (*Answer, error) {
	if provideFn == nil {
		provideFn = _FakeProvideFn
	}

	if provideFn == nil || msg == nil || len(msg.Questions) == 0 {
		return nil, errors.New("invalid param")
	}

	q := &msg.Questions[0]

	name := strings.TrimSuffix(q.Name.String(), ".")
	ansIPs := provideFn(name, q.Type)
	if len(ansIPs) == 0 {
		return nil, errors.New("no answer IPs")
	}

	newMsg := &dnsmessage.Message{}

	*newMsg = *msg
	newMsg.Authorities = nil
	newMsg.Additionals = nil
	newMsg.Answers = nil
	newMsg.Header.Response = true

	for i := 0; i < len(ansIPs); i++ {
		ip := net.ParseIP(ansIPs[i])
		if len(ip) == 0 {
			continue
		}

		var res dnsmessage.Resource
		res.Header.Class = dnsmessage.ClassINET
		res.Header.Type = q.Type
		res.Header.Name = q.Name
		res.Header.TTL = 30

		switch q.Type {
		case dnsmessage.TypeA:
			if ip4 := ip.To4(); ip4 != nil {
				r := &dnsmessage.AResource{}
				copy(r.A[:], ip4)
				res.Body = r
				newMsg.Answers = append(newMsg.Answers, res)
			}
		case dnsmessage.TypeAAAA:
			if ip6 := ip.To16(); ip6 != nil {
				r := &dnsmessage.AAAAResource{}
				copy(r.AAAA[:], ip6)
				res.Body = r
				newMsg.Answers = append(newMsg.Answers, res)
			}
		default:
		}
	}

	if len(newMsg.Answers) == 0 {
		return nil, errors.New("no answer IPs")
	}

	answer := &Answer{Msg: newMsg}

	// println("len(newMsg.Answers) =", len(newMsg->Answers));

	return answer, nil

}
