//
// weproxy@foxmail.com 2022/10/20
//

package server

import (
	"context"
	"errors"
	"net"
	"syscall"
	"time"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/stack/netstk"
	"weproxy/acc/libgo/nx/util"
)

// tproxyServer ...
type tproxyServer struct {
	h     netstk.Handler
	tcpln net.Listener
	udpln net.PacketConn
}

// Start addr 10.20.30.40:6666
func (m *tproxyServer) Start(laddr string) (err error) {
	lc := net.ListenConfig{
		Control: func(network, address string, c syscall.RawConn) error {
			return c.Control(func(fd uintptr) {
				util.SetOptTransparent(int(fd), 1)
				util.SetOptRecvOrigDst(int(fd), 1)
			})
		},
	}

	// TCP
	m.tcpln, err = lc.Listen(context.Background(), "tcp", laddr)
	if err != nil {
		logx.E("%s TCP listen on: %v, %v", TAG, laddr, err)
		return
	}
	logx.I("%s TCP start on: %v", TAG, laddr)

	// UDP
	m.udpln, err = lc.ListenPacket(context.Background(), "udp", laddr)
	if err != nil {
		logx.E("%s UDP listen on: %v, %v", TAG, laddr, err)
		return
	}
	logx.I("%s UDP start on: %v", TAG, laddr)

	// loopAccept
	go m.loopAccept()

	// loopRecvFrom
	go m.loopRecvFrom()

	return
}

// Close ...
func (m *tproxyServer) Close() error {
	if m.udpln != nil {
		m.udpln.Close()
	}
	if m.tcpln != nil {
		m.tcpln.Close()
	}
	return nil
}

////////////////////////////////////////////////////////////////////////////////

// tproxyConn implements netstk.Conn
type tproxyConn struct {
	*net.TCPConn
	id      uint64
	origDst net.Addr
}

// ID ...
func (m *tproxyConn) ID() uint64 {
	return m.id
}

// RemoteAddr ...
func (m *tproxyConn) RemoteAddr() net.Addr {
	return m.origDst
}

//----------------------------------------------------------

// loopAccept ...
func (m *tproxyServer) loopAccept() {
	ln := m.tcpln.((*net.TCPListener))
	for {
		c, err := ln.AcceptTCP()
		if err != nil {
			if !errors.Is(err, net.ErrClosed) {
				logx.E("%s, ln.Accept(), err: ", TAG, err)
			}
			break
		}

		// get origin dst
		origDst, err := util.GetTCPOriginDst(c)
		if err != nil {
			logx.W("%s get TCP %v origin dst fail: %v", TAG, c.LocalAddr(), err)
			c.Close()
			continue
		}

		// handle it
		cc := &tproxyConn{
			TCPConn: c,
			id:      nx.NewID(),
			origDst: origDst,
		}
		go m.h.Handle(cc)
	}
}

////////////////////////////////////////////////////////////////////////////////

// tproxyPacketConn implements netstk.PacketConn
type tproxyPacketConn struct {
	ln           net.PacketConn
	id           uint64
	laddr, raddr net.Addr
	data         chan *nx.Packet
	closed       chan struct{}
}

// ID ...
func (m *tproxyPacketConn) ID() uint64 {
	return m.id
}

// ReadFrom ...
func (m *tproxyPacketConn) ReadFrom(p []byte) (n int, addr net.Addr, err error) {
	select {
	case <-m.closed:
		return 0, nil, net.ErrClosed
	case p, ok := <-m.data:
		if !ok {
			return
		}
		return len(p.Data), p.Addr, nil
	}
}

// WriteTo ...
func (m *tproxyPacketConn) WriteTo(p []byte, addr net.Addr) (n int, err error) {
	select {
	case <-m.closed:
		return 0, net.ErrClosed
	default:
	}
	return m.ln.WriteTo(p, addr)
}

// Close ...
func (m *tproxyPacketConn) Close() error {
	select {
	case <-m.closed:
	default:
		close(m.closed)
	}
	return nil
}

// LocalAddr ...
func (m *tproxyPacketConn) LocalAddr() net.Addr {
	return m.laddr
}

// RemoteAddr ...
func (m *tproxyPacketConn) RemoteAddr() net.Addr {
	return m.raddr
}

// SetDeadline ...
func (m *tproxyPacketConn) SetDeadline(t time.Time) error {
	// TODO ...
	return nil
}

// SetReadDeadline ...
func (m *tproxyPacketConn) SetReadDeadline(t time.Time) error {
	// TODO ...
	return nil
}

// SetWriteDeadline ...
func (m *tproxyPacketConn) SetWriteDeadline(t time.Time) error {
	// TODO ...
	return nil
}

//----------------------------------------------------------

// _udps ...
var _udps = make(map[nx.Key]*tproxyPacketConn)

//----------------------------------------------------------

// loopRecvFrom ...
func (m *tproxyServer) loopRecvFrom() {
	buf := make([]byte, 8*1024)
	oob := make([]byte, 1024)

	ln := m.udpln.(*net.UDPConn)
	for {
		nr, oobn, _, replyDst, err := ln.ReadMsgUDP(buf, oob)
		if err != nil {
			if !errors.Is(err, net.ErrClosed) {
				logx.E("%s, ln.ReadMsgUDP(), err: ", TAG, err)
			}
			break
		}

		// get origin dst
		origDst, err := util.GetUDPOriginDst(oob[:oobn])
		if err != nil {
			logx.W("%s get UDP %v origin dst fail: %v", TAG, replyDst, err)
			continue
		}

		// clone
		data := append([]byte(nil), buf[:nr]...)

		// lookfor
		key := nx.MakeKey(replyDst)
		pc, ok := _udps[key]
		if !ok {
			// new session
			pc = &tproxyPacketConn{
				ln:     ln,
				id:     nx.NewID(),
				laddr:  replyDst,
				raddr:  origDst,
				data:   make(chan *nx.Packet, 256),
				closed: make(chan struct{}),
			}
			_udps[key] = pc

			// handle it
			go m.h.HandlePacket(pc)
		}

		// push in it's chan without blocking this loop
		select {
		case pc.data <- &nx.Packet{Addr: origDst, Data: data}:
		default:
		}
	}
}
