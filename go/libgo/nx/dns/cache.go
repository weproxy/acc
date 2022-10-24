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

	q := &msg.Questions[0]

	logx.I("[dns] cache.Query %v%v", q.Name, q.Type)

	key := makeCacheKey(q)

	m.mu.Lock()
	defer m.mu.Unlock()

	entry, ok := m.mem[key]
	if !ok || entry == nil || entry.msg == nil {
		return
	}
	if time.Now().After(entry.exp) {
		delete(m.mem, key)
		return
	}

	amsg := &dnsmessage.Message{}
	*amsg = *entry.msg
	amsg.Header.ID = msg.Header.ID
	amsg.Header.Response = true

	data, err := amsg.Pack()
	if err != nil {
		return nil, err
	}

	answer = &Answer{
		Msg:  amsg,
		Name: strings.TrimSuffix(q.Name.String(), "."),
		Data: data,
	}

	logx.W("[dns] cache.Answer %v%v <- %v", q.Name, q.Type, toAnswerString(answer.Msg.Answers))

	return
}

// store ...
func (m *cacheMap) store(msg *dnsmessage.Message) (err error) {
	if msg == nil || msg.RCode != dnsmessage.RCodeSuccess || len(msg.Questions) == 0 {
		return
	}

	q := msg.Questions[0]

	key := makeCacheKey(&q)
	exp := time.Now().Add(time.Second * 300)

	m.mu.Lock()
	m.mem[key] = &cacheEntry{msg: msg, exp: exp}
	m.mu.Unlock()

	logx.W("[dns] cache.Store %v%v <- %v", q.Name, q.Type, toAnswerString(msg.Answers))

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
