//
// weproxy@foxmail.com 2022/10/20
//

package stats

import (
	"fmt"
	"sync/atomic"
	"time"
	"weproxy/acc/libgo/fx"
	"weproxy/acc/libgo/logx"
)

// Type ...
type Type int

const (
	TypeDirect Type = iota
	TypeS5
	TypeSS
	TypeGAAP
	TypeHTP
	TypeDNS
	TypeKCP
	TypeQUIC
	TypeOTHER
	typeMax
)

// typeCount ...
const typeCount = typeMax - TypeDirect

// String ...
func (m Type) String() string {
	return []string{"Direct", "S5", "SS", "GAAP", "HTP", "DNS", "KC", "QC", "OTH", ""}[m-TypeDirect]
}

// /////////////////////////////////////////////////////////////////////////////////////////////////
// Conn  ...
var Conn *connT = newConnT()

// Rate  ...
var Rate *rateT = newRateT()

// DNS  ...
var DNS *dnsT = newDnsT()

///////////////////////////////////////////////////////////////////////////////////////////////////

const TAG = "[stats]"

// init ...
func init() {
	go func() {
		t := time.NewTicker(time.Second)
		for range t.C {
			Rate.calc()
			DNS.calc()
			Conn.calc()
		}
	}()
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// Stats ...
type Stats struct {
	sent, recv   int64
	sentx, recvx int64
	tcp          bool
	start        time.Time
	tag          string
	typ          Type
	conn         *connT
	rate         *rateT
	target       string
	server       string
}

// newStats ...
func newStats(typ Type, tag string, tcp bool) *Stats {
	return &Stats{
		tcp:  tcp,
		tag:  tag,
		typ:  typ,
		conn: Conn,
		rate: Rate,
	}
}

// NewTCPStats ...
func NewTCPStats(typ Type, tag string) *Stats {
	return newStats(typ, tag, true)
}

// NewUDPStats ...
func NewUDPStats(typ Type, tag string) *Stats {
	return newStats(typ, tag, false)
}

// Tag ...
func (m *Stats) Tag() string {
	return m.tag
}

// SetTag ...
func (m *Stats) SetTag(tag string) *Stats {
	m.tag = tag
	return m
}

// Target ...
func (m *Stats) Target() string {
	return m.target
}

// SetTarget ...
func (m *Stats) SetTarget(tgt string) *Stats {
	m.target = tgt
	return m
}

// Server ...
func (m *Stats) Server() string {
	return m.server
}

// SetServer ...
func (m *Stats) SetServer(svr string) *Stats {
	m.server = svr
	return m
}

// SetProxy ...
func (m *Stats) SetProxy(svr, tgt string) *Stats {
	m.server = svr
	m.target = tgt
	return m
}

// AddSent ..
func (m *Stats) AddSent(n int64) (new int64) {
	if m.tcp {
		m.rate.AddTCPSent(m.typ, n)
	} else {
		m.rate.AddUDPSent(m.typ, n)
	}
	return atomic.AddInt64(&m.sent, n)
}

// AddRecv ..
func (m *Stats) AddRecv(n int64) (new int64) {
	if m.tcp {
		m.rate.AddTCPRecv(m.typ, n)
	} else {
		m.rate.AddUDPRecv(m.typ, n)
	}
	return atomic.AddInt64(&m.recv, n)
}

// AddSentX ..
func (m *Stats) AddSentX(n int64) (new int64) {
	if m.tcp {
		m.rate.AddTCPSent(m.typ, n)
	} else {
		m.rate.AddUDPSent(m.typ, n)
	}
	return atomic.AddInt64(&m.sentx, n)
}

// AddRecvX ..
func (m *Stats) AddRecvX(n int64) (new int64) {
	if m.tcp {
		m.rate.AddTCPRecv(m.typ, n)
	} else {
		m.rate.AddUDPRecv(m.typ, n)
	}
	return atomic.AddInt64(&m.recvx, n)
}

// Elapsed ..
func (m *Stats) Elapsed() time.Duration {
	return time.Since(m.start)
}

// LogD ..
func (m *Stats) LogD(format string, args ...interface{}) {
	logx.D("%v %v, %v", m.tag, fmt.Sprintf(format, args...), m.Elapsed())
}

// LogI ..
func (m *Stats) LogI(format string, args ...interface{}) {
	logx.I("%v %v, %v", m.tag, fmt.Sprintf(format, args...), m.Elapsed())
}

// LogW ..
func (m *Stats) LogW(format string, args ...interface{}) {
	logx.W("%v %v, %v", m.tag, fmt.Sprintf(format, args...), m.Elapsed())
}

// LogE ..
func (m *Stats) LogE(format string, args ...interface{}) {
	logx.W("%v %v, %v", m.tag, fmt.Sprintf(format, args...), m.Elapsed())
}

// Start ...
func (m *Stats) Start(format string, args ...interface{}) *Stats {
	m.start = time.Now()

	logx.I("%v %v", m.tag, fmt.Sprintf(format, args...))

	if m.tcp {
		m.conn.AddTCP(m.typ, 1)
	} else {
		m.conn.AddUDP(m.typ, 1)
	}
	return m
}

// Done ...
func (m *Stats) Done(format string, args ...interface{}) *Stats {
	if m.tcp {
		m.conn.AddTCP(m.typ, -1)
	} else {
		m.conn.AddUDP(m.typ, -1)
	}

	var logmsg string

	if m.sentx > 0 || m.recvx > 0 {
		logmsg = fmt.Sprintf("%v %v, sent=%v+%v, recv=%v+%v, %v",
			m.tag, fmt.Sprintf(format, args...),
			fx.FormatBytes(m.sentx), fx.FormatBytes(m.sent),
			fx.FormatBytes(m.recvx), fx.FormatBytes(m.recv),
			m.Elapsed())
	} else {
		logmsg = fmt.Sprintf("%v %v, sent=%v, recv=%v, %v",
			m.tag, fmt.Sprintf(format, args...),
			fx.FormatBytes(m.sent), fx.FormatBytes(m.recv),
			m.Elapsed())
	}

	logx.I(logmsg)

	return m
}
