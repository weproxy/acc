//
// weproxy@foxmail.com 2022/10/20
//

package tproxy

import (
	"errors"
	"net"
	"sync"
	"time"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx"
	"weproxy/acc/libgo/nx/util"
)

////////////////////////////////////////////////////////////////////////////////

// stackPacketConn implements netstk.PacketConn
type stackPacketConn struct {
	ln           net.PacketConn
	id           uint64
	key          nx.Key
	laddr, raddr net.Addr
	data         chan *nx.Packet
	closed       chan struct{}
}

// ID ...
func (m *stackPacketConn) ID() uint64 {
	return m.id
}

// ReadFrom ...
func (m *stackPacketConn) ReadFrom(p []byte) (n int, addr net.Addr, err error) {
	select {
	case <-m.closed:
	case pkt, ok := <-m.data:
		if ok {
			return copy(p, pkt.Data), pkt.Addr, nil
		}
	}
	return 0, nil, net.ErrClosed
}

// WriteTo ...
func (m *stackPacketConn) WriteTo(p []byte, addr net.Addr) (n int, err error) {
	select {
	case <-m.closed:
		return 0, net.ErrClosed
	default:
	}
	return m.ln.WriteTo(p, addr)
}

// Close ...
func (m *stackPacketConn) Close() error {
	select {
	case <-m.closed:
	default:
		_mudp.Lock()
		delete(_udps, m.key)
		_mudp.Unlock()
		close(m.closed)
	}
	return nil
}

// LocalAddr ...
func (m *stackPacketConn) LocalAddr() net.Addr {
	return m.laddr
}

// RemoteAddr ...
func (m *stackPacketConn) RemoteAddr() net.Addr {
	return m.raddr
}

// SetDeadline ...
func (m *stackPacketConn) SetDeadline(t time.Time) error {
	// TODO ...
	return nil
}

// SetReadDeadline ...
func (m *stackPacketConn) SetReadDeadline(t time.Time) error {
	// TODO ...
	return nil
}

// SetWriteDeadline ...
func (m *stackPacketConn) SetWriteDeadline(t time.Time) error {
	// TODO ...
	return nil
}

////////////////////////////////////////////////////////////////////////////////

// _udps ...
var _udps = make(map[nx.Key]*stackPacketConn)
var _mudp sync.Mutex

////////////////////////////////////////////////////////////////////////////////

// loopRecvFrom ...
func (m *Server) loopRecvFrom() {
	buf := make([]byte, 1024*8)
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
		_mudp.Lock()
		pc, ok := _udps[key]
		_mudp.Unlock()
		if !ok {
			// new session
			pc = &stackPacketConn{
				ln:     ln,
				id:     nx.NewID(),
				laddr:  replyDst,
				raddr:  origDst,
				data:   make(chan *nx.Packet, 256),
				closed: make(chan struct{}),
			}
			_mudp.Lock()
			_udps[key] = pc
			_mudp.Unlock()

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
