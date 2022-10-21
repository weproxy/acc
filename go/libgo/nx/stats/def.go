//
// weproxy@foxmail.com 2022/10/20
//

package stats

import (
	"bytes"
	"fmt"
	"sync"
	"sync/atomic"
	"weproxy/acc/libgo/fx"
	"weproxy/acc/libgo/logx"
)

// connE ...
// notice: armv7  panic: unaligned 64-bit atomic operation
type connE struct {
	tcp, tcpTotal int64
	udp, udpTotal int64
}

// String ...
func (m *connE) String() string {
	return fmt.Sprintf("T:%v/%v,U:%v/%v", m.tcp, m.tcpTotal, m.udp, m.udpTotal)
}

// connT ...
type connT struct {
	dirty int32
	arr   [typeCount]*connE
}

// newConnT ...
func newConnT() *connT {
	m := &connT{}
	for i := 0; i < int(typeCount); i++ {
		m.arr[i] = &connE{}
	}
	return m
}

// AddTCP ...
func (m *connT) AddTCP(typ Type, delta int64) int64 {
	atomic.SwapInt32(&m.dirty, 1)
	if delta > 0 {
		atomic.AddInt64(&m.arr[typ].tcpTotal, delta)
	}
	return atomic.AddInt64(&m.arr[typ].tcp, delta)
}

// AddUDP ...
func (m *connT) AddUDP(typ Type, delta int64) int64 {
	atomic.SwapInt32(&m.dirty, 1)
	if delta > 0 {
		atomic.AddInt64(&m.arr[typ].udpTotal, delta)
	}
	return atomic.AddInt64(&m.arr[typ].udp, delta)
}

// GetTCP ...
func (m *connT) GetTCP(typ Type) int64 {
	return atomic.LoadInt64(&m.arr[typ].tcp)
}

// GetUDP ...
func (m *connT) GetUDP(typ Type) int64 {
	return atomic.LoadInt64(&m.arr[typ].udp)
}

// GetTCPTotal ...
func (m *connT) GetTCPTotal(typ Type) int64 {
	return atomic.LoadInt64(&m.arr[typ].tcpTotal)
}

// GetUDPTotal ...
func (m *connT) GetUDPTotal(typ Type) int64 {
	return atomic.LoadInt64(&m.arr[typ].udpTotal)
}

// calc ...
func (m *connT) calc() {
	dirty := atomic.SwapInt32(&m.dirty, 0)
	if dirty == 0 {
		return
	}

	var bb bytes.Buffer

	bb.WriteString("[stats] Conn{cur/his}:")

	for i, it := range m.arr {
		typ := Type(int(TypeDirect) + i)
		if typ == typeMax || (it.tcpTotal == 0 && it.udpTotal == 0) {
			continue
		}
		bb.WriteString(fmt.Sprintf(" %v{%v}", typ, it))
	}

	logx.P(bb.String())
}

///////////////////////////////////////////////////////////////////////////////////////////////////

const (
	_dnsFlagSucceeded int = iota
	_dnsFlagFailed
	_dnsFlagCacheHit
	_dnsFlagDropped
	_dnsFlagFaked
)

///////////////////////////////////////////////////////////////////////////////////////////////////

// dnsI ...
type dnsI struct {
	flg int
	cnt int
}

// setbit ...
func (m *dnsI) setbit(bit int, b bool) *dnsI {
	if b {
		m.flg |= 1 << bit
	} else {
		m.flg &= ^(1 << bit)
	}
	return m
}

// getbit ...
func (m *dnsI) getbit(bit int) bool {
	return (m.flg & (1 << bit)) != 0
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// dnsE ...
type dnsE struct {
	total     int64
	succeeded int64
	failed    int64
	cahcehit  int64
	dropped   int64
	faked     int64
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// dnsT ...
type dnsT struct {
	all      *dnsE
	distinct *dnsE
	names    sync.Map
	dirty    int32
}

// newDnsT ...
func newDnsT() *dnsT {
	m := &dnsT{
		all:      &dnsE{},
		distinct: &dnsE{},
	}
	return m
}

// AddQuery ...
func (m *dnsT) AddQuery(name ...string) {
	if len(name) == 0 {
		return
	}
	atomic.SwapInt32(&m.dirty, 1)

	atomic.AddInt64(&m.all.total, int64(len(name)))
	for _, s := range name {
		if v, ok := m.names.Load(s); !ok {
			atomic.AddInt64(&m.distinct.total, 1)
			m.names.Store(s, &dnsI{cnt: 1})
		} else if p, ok := v.(*dnsI); ok {
			p.cnt++
		}
	}
}

// add ...
func (m *dnsT) add(all, distinct *int64, bit int, name ...string) {
	if len(name) == 0 {
		return
	}
	atomic.SwapInt32(&m.dirty, 1)

	atomic.AddInt64(all, int64(len(name)))
	for _, s := range name {
		if v, ok := m.names.Load(s); !ok {
			atomic.AddInt64(distinct, 1)
		} else if p, ok := v.(*dnsI); ok && !p.getbit(bit) {
			p.setbit(bit, true)
			atomic.AddInt64(distinct, 1)
		}
	}
}

// AddSucceeded ...
func (m *dnsT) AddSucceeded(name ...string) {
	m.add(&m.all.succeeded, &m.distinct.succeeded, _dnsFlagSucceeded, name...)
}

// AddFailed ...
func (m *dnsT) AddFailed(name ...string) {
	m.add(&m.all.failed, &m.distinct.failed, _dnsFlagFailed, name...)
}

// AddCacheHit ...
func (m *dnsT) AddCacheHit(name ...string) {
	m.add(&m.all.cahcehit, &m.distinct.cahcehit, _dnsFlagCacheHit, name...)
}

// AddDropped ...
func (m *dnsT) AddDropped(name ...string) {
	m.add(&m.all.dropped, &m.distinct.dropped, _dnsFlagDropped, name...)
}

// AddFaked ...
func (m *dnsT) AddFaked(name ...string) {
	m.add(&m.all.faked, &m.distinct.faked, _dnsFlagFaked, name...)
}

// calc ...
func (m *dnsT) calc() {
	dirty := atomic.SwapInt32(&m.dirty, 0)
	if dirty == 0 {
		return
	}

	all, dis := m.all, m.distinct

	logmsg := fmt.Sprintf("[stats] DNS{dist/indist}: total=%v/%v, succ=%v/%v, fail=%v/%v, hit=%v/%v, drop=%v/%v, fake=%v/%v",
		atomic.LoadInt64(&dis.total), atomic.LoadInt64(&all.total),
		atomic.LoadInt64(&dis.succeeded), atomic.LoadInt64(&all.succeeded),
		atomic.LoadInt64(&dis.failed), atomic.LoadInt64(&all.failed),
		atomic.LoadInt64(&dis.cahcehit), atomic.LoadInt64(&all.cahcehit),
		atomic.LoadInt64(&dis.dropped), atomic.LoadInt64(&all.dropped),
		atomic.LoadInt64(&dis.faked), atomic.LoadInt64(&all.faked),
	)

	logx.P(logmsg)
}

type rateE struct {
	sent, sentTotal int64
	recv, recvTotal int64
}

// calc ...
func (m *rateE) calc(typ Type, tcp bool) {
	sentRate := atomic.SwapInt64(&m.sent, 0)
	recvRate := atomic.SwapInt64(&m.recv, 0)

	tag := "TCP"
	if !tcp {
		tag = "UDP"
	}

	if sentRate > 0 || recvRate > 0 {
		logmsg := fmt.Sprintf("[rate] %v %v: sent=%v/s, recv=%v/s", typ, tag,
			fx.FormatBytes(sentRate), fx.FormatBytes(recvRate))

		logx.P(logmsg)
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// rateT ...
type rateT struct {
	tcp [typeCount]*rateE // TCP
	udp [typeCount]*rateE // UDP
}

// newRateT ...
func newRateT() *rateT {
	m := &rateT{}
	for i := 0; i < int(typeCount); i++ {
		m.tcp[i] = &rateE{}
		m.udp[i] = &rateE{}
	}
	return m
}

// AddTCPSent ...
func (m *rateT) AddTCPSent(typ Type, delta int64) {
	p := m.tcp[typ]
	atomic.AddInt64(&p.sent, delta)
	atomic.AddInt64(&p.sentTotal, delta)
}

// AddTCPRecv ...
func (m *rateT) AddTCPRecv(typ Type, delta int64) {
	p := m.tcp[typ]
	atomic.AddInt64(&p.recv, delta)
	atomic.AddInt64(&p.recvTotal, delta)
}

// AddUDPSent ...
func (m *rateT) AddUDPSent(typ Type, delta int64) {
	p := m.udp[typ]
	atomic.AddInt64(&p.sent, delta)
	atomic.AddInt64(&p.sentTotal, delta)
}

// AddUDPRecv ...
func (m *rateT) AddUDPRecv(typ Type, delta int64) {
	p := m.udp[typ]
	atomic.AddInt64(&p.recv, delta)
	atomic.AddInt64(&p.recvTotal, delta)
}

// calc ...
func (m *rateT) calc() {
	for i, p := range m.tcp {
		p.calc(Type(i), true)
	}
	for i, p := range m.udp {
		p.calc(Type(i), false)
	}
}
