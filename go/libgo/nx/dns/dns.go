//
// weproxy@foxmail.com 2022/10/22
//

package dns

import (
	"errors"
	"fmt"
	"net"
	"strings"

	"golang.org/x/net/dns/dnsmessage"
)

// ...
var (
	ErrQueryNotFound = errors.New("not found")
	ErrQueryDropped  = errors.New("dropped")
	ErrQueryFaked    = errors.New("faked")
	ErrQueryHited    = errors.New("hited")
)

////////////////////////////////////////////////////////////////////////////////

var (
	_cache = newCacheMap()
)

////////////////////////////////////////////////////////////////////////////////

// Answer ...
type Answer struct {
	Msg *dnsmessage.Message
}

// Name ...
func (m *Answer) Name() string {
	if m.Msg != nil && len(m.Msg.Questions) > 0 {
		return strings.TrimSuffix(m.Msg.Questions[0].Name.String(), ".")
	}
	return ""
}

// Bytes ...
func (m *Answer) Type() dnsmessage.Type {
	if m.Msg != nil && len(m.Msg.Questions) > 0 {
		return m.Msg.Questions[0].Type
	}
	return dnsmessage.TypeA
}

// Bytes ...
func (m *Answer) Bytes() []byte {
	if m.Msg != nil {
		if b, err := m.Msg.Pack(); err == nil {
			return b
		}
	}
	return nil
}

////////////////////////////////////////////////////////////////////////////////

// OnRequest ...
func OnRequest(b []byte) (msg *dnsmessage.Message, answer *Answer, err error) {
	msg = &dnsmessage.Message{}
	if err = msg.Unpack(b); err == nil {
		answer, err = _cache.load(msg)
	}
	return
}

// OnResponse ...
func OnResponse(b []byte) (msg *dnsmessage.Message, err error) {
	msg = &dnsmessage.Message{}
	if err = msg.Unpack(b); err == nil {
		err = _cache.store(msg)
	}
	if err != nil {
		return nil, err
	}
	return
}

////////////////////////////////////////////////////////////////////////////////

// toAnswerString ...
func toAnswerString(arr []dnsmessage.Resource) string {
	var answs []string

	for _, answ := range arr {
		if answ.Body == nil {
			continue
		}
		switch answ.Header.Type {
		case dnsmessage.TypeA:
			answs = append(answs, fmt.Sprintf("%v", net.IP(answ.Body.(*dnsmessage.AResource).A[:])))
		case dnsmessage.TypeAAAA:
			answs = append(answs, fmt.Sprintf("%v", net.IP(answ.Body.(*dnsmessage.AAAAResource).AAAA[:])))
		case dnsmessage.TypeCNAME:
			answs = append(answs, strings.TrimRight(answ.Body.(*dnsmessage.CNAMEResource).CNAME.String(), "."))
		// case dnsmessage.TypeNS:
		// 	answs = append(answs, answ.Body.(*dnsmessage.NSResource).NS.String()+"NS")
		default:
			answs = append(answs, fmt.Sprintf("Type%v", int(answ.Header.Type)))
		}
	}

	return fmt.Sprintf("%v", answs)
}
