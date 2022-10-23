//
// weproxy@foxmail.com 2022/10/22
//

package dns

import (
	"runtime"
	"strings"
	"sync"
	"time"

	"golang.org/x/net/dns/dnsmessage"

	"weproxy/acc/libgo/logx"
)

// cacheEntry ...
type cacheEntry struct {
	msg *dnsmessage.Message
	exp time.Time
}

// cacheMap ...
type cacheMap struct {
	mu  sync.Mutex
	mem map[string]*cacheEntry
}

// makeCacheKey ...
func makeCacheKey(q *dnsmessage.Question) string {
	return string(q.Name.String() + q.Type.String())
}

// newCacheMap ...
func newCacheMap() *cacheMap {
	cache := &cacheMap{mem: make(map[string]*cacheEntry)}
	go cache.cleanUp()
	return cache
}

// load ...
func (m *cacheMap) load(msg *dnsmessage.Message) (answer *Answer, err error) {
	if msg == nil || len(msg.Questions) == 0 {
		return
	}

	logx.I("[dns] cache.Query %v%v", msg.Questions[0].Name, msg.Questions[0].Type)

	key := makeCacheKey(&msg.Questions[0])

	m.mu.Lock()
	defer m.mu.Unlock()

	entry, ok := m.mem[key]
	if !ok {
		return
	}
	if time.Now().After(entry.exp) {
		delete(m.mem, key)
		return
	}

	amsg := &dnsmessage.Message{}
	amsg.Answers = entry.msg.Answers
	amsg.Additionals = entry.msg.Additionals
	amsg.Authorities = entry.msg.Authorities

	amsg.Header.ID = msg.Header.ID
	amsg.Header.Response = true
	amsg.Header.RCode = entry.msg.Header.RCode
	amsg.Header.OpCode = entry.msg.Header.OpCode
	amsg.Header.Authoritative = entry.msg.Header.Authoritative
	amsg.Header.Truncated = entry.msg.Header.Truncated
	amsg.Header.RecursionDesired = entry.msg.Header.RecursionDesired
	amsg.Header.RecursionAvailable = entry.msg.Header.RecursionAvailable

	data, err := amsg.Pack()
	if err != nil {
		return nil, err
	}

	answer = &Answer{
		Msg:  amsg,
		Name: strings.TrimSuffix(msg.Questions[0].Name.String(), "."),
		Data: data,
	}

	logx.W("[dns] cache.Answer %v%v <- %v", msg.Questions[0].Name, msg.Questions[0].Type, toAnswerString(answer.Msg.Answers))

	return
}

// store ...
func (m *cacheMap) store(msg *dnsmessage.Message) (err error) {
	if msg == nil || msg.RCode != dnsmessage.RCodeSuccess || len(msg.Questions) == 0 {
		return
	}

	key := makeCacheKey(&msg.Questions[0])
	exp := time.Now().Add(time.Second * 300)

	m.mu.Lock()
	m.mem[key] = &cacheEntry{msg: msg, exp: exp}
	m.mu.Unlock()

	logx.W("[dns] cache.Store %v%v <- %v", msg.Questions[0].Name, msg.Questions[0].Type, toAnswerString(msg.Answers))

	return
}

// Clear ...
func (m *cacheMap) Clear() {
	m.mu.Lock()
	m.mem = make(map[string]*cacheEntry)
	m.mu.Unlock()
}

// cleanUp ...
func (m *cacheMap) cleanUp() {
	dura := 30 * time.Second
	if runtime.GOOS == "darwin" {
		dura = 10 * time.Second
	}

	cleanFn := func() {
		var delKeys []string
		for key, entry := range m.mem {
			if !time.Now().Before(entry.exp) {
				delKeys = append(delKeys, key)
			}
		}
		if len(delKeys) > 0 {
			for _, key := range delKeys {
				delete(m.mem, key)
			}
			logx.W("[dns] cache.cleanUp %v, remain %v entries", len(delKeys), len(m.mem))
		}
	}

	t := time.NewTicker(dura)
	defer t.Stop()

	for range t.C {
		m.mu.Lock()
		if len(m.mem) > 0 {
			cleanFn()
		}
		m.mu.Unlock()
	}
}
