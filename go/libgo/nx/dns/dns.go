//
// weproxy@foxmail.com 2022/10/22
//

package dns

import "golang.org/x/net/dns/dnsmessage"

var (
	Cache = newCacheMap()
)

// Answer ...
type Answer struct {
	Name string
	Data []byte
	Msg  *dnsmessage.Message
}

// OnRequest ...
func (m *cacheMap) OnRequest(b []byte) (msg *dnsmessage.Message, answer *Answer, err error) {
	msg = &dnsmessage.Message{}
	if err = msg.Unpack(b); err == nil {
		answer, err = m.load(msg)
	}
	return
}

// OnResponse ...
func (m *cacheMap) OnResponse(b []byte) (msg *dnsmessage.Message, err error) {
	msg = &dnsmessage.Message{}
	if err = msg.Unpack(b); err == nil {
		err = m.store(msg)
	}
	if err != nil {
		return nil, err
	}
	return
}
