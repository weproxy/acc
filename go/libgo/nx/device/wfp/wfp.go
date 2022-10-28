//
// weproxy@foxmail.com 2022/10/20
//

package wfp

import (
	"errors"
	"fmt"
	"io"
	"net"
	"sync"
	"time"

	"golang.org/x/net/ipv4"
	"golang.org/x/net/ipv6"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/device"
	"weproxy/acc/libgo/nx/device/wfp/windivert"
	"weproxy/acc/libgo/nx/stack/netstk"
)

/*
https://reqrypt.org/windivert.html
https://github.com/basil00/Divert
https://github.com/imgk/divert-go

* Support WinDivert 2.x
* Optional CGO support to remove dependence of WinDivert.dll, use -tags="divert_cgo"
* Support loading dll from rsrc data, use -tags="divert_embedded"
*/

// init ...
func init() {
	device.Register(device.TypeWFP, New)
}

// New ...
func New(cfg device.Conf) (netstk.Device, error) {
	return nil, errors.New("wfp.New() not impl")
}

////////////////////////////////////////////////////////////////////////////////

// Device implements netstk.Device
type Device struct {
	addr   *windivert.Address
	h      *windivert.Handle
	filter *PacketFilter
	p      struct {
		*io.PipeReader
		*io.PipeWriter
		eventCh chan struct{}
	}
	closedCh chan struct{}
}

// Type ...
func (m *Device) Type() string {
	return device.TypeWFP
}

// Close implements io.Closer
func (m *Device) Close() error {
	select {
	case <-m.closedCh:
		return nil
	default:
		close(m.closedCh)
	}

	// close io.PipeReader and io.PipeWriter
	m.p.PipeReader.Close()
	m.p.PipeWriter.Close()

	// close windivert.Handle
	if err := m.h.Shutdown(windivert.ShutdownBoth); err != nil {
		m.h.Close()
		return fmt.Errorf("shutdown handle error: %w", err)
	}
	if err := m.h.Close(); err != nil {
		return fmt.Errorf("close handle error: %w", err)
	}
	return nil
}

// Read from device
func (m *Device) Read(p []byte, offset int) (n int, err error) {
	err = errors.New("windows divert hasn't Read")
	panic(err)
	return
}

// Write to device
func (m *Device) Write(p []byte, offset int) (n int, err error) {
	select {
	case <-m.closedCh:
		return 0, io.EOF
	case m.p.eventCh <- struct{}{}:
	}

	n, err = m.p.Write(p[offset:])
	if err != nil {
		select {
		case <-m.closedCh:
			return 0, io.EOF
		default:
		}
	}

	return n, err
}

// loop is ...
func (m *Device) loop() (err error) {
	t := time.NewTicker(time.Millisecond)
	defer t.Stop()

	const flags = uint8(0x01<<7) | uint8(0x01<<6) | uint8(0x01<<5)

	addr := make([]windivert.Address, windivert.BatchMax)
	buff := make([]byte, 1500*windivert.BatchMax)

	for i := range addr {
		addr[i] = *m.addr
		addr[i].Flags |= flags
	}

	nb, nx := 0, 0
LOOP:
	for {
		select {
		case <-t.C:
			if nx > 0 {
				m.h.Lock()
				_, ew := m.h.SendEx(buff[:nb], addr[:nx])
				m.h.Unlock()
				if ew != nil {
					err = fmt.Errorf("device loop error: %w", ew)
					break LOOP
				}
				nb, nx = 0, 0
			}
		case <-m.p.eventCh:
			nr, er := m.p.Read(buff[nb:])
			if er != nil {
				err = fmt.Errorf("device loop error: %w", er)
				break LOOP
			}

			nb += nr
			nx++

			if nx < windivert.BatchMax {
				continue
			}

			m.h.Lock()
			_, ew := m.h.SendEx(buff[:nb], addr[:nx])
			m.h.Unlock()
			if ew != nil {
				err = fmt.Errorf("device loop error: %w", ew)
				break LOOP
			}
			nb, nx = 0, 0
		case <-m.closedCh:
			return
		}
	}
	if err != nil {
		select {
		case <-m.closedCh:
		default:
			// log.Panic(err)
			logx.E("[wfp] %v", err)
		}
	}
	return nil
}

// WriteTo ...
func (m *Device) WriteTo(w io.Writer) (n int64, err error) {
	addr := make([]windivert.Address, windivert.BatchMax)
	buff := make([]byte, 1500*windivert.BatchMax)

	const flags = uint8(0x01<<7) | uint8(0x01<<6) | uint8(0x01<<5) | uint8(0x01<<3)
	for {
		nb, nx, er := m.h.RecvEx(buff, addr)
		if er != nil {
			err = fmt.Errorf("handle recv error: %w", er)
			break
		}
		if nb < 1 || nx < 1 {
			continue
		}

		n += int64(nb)

		bb := buff[:nb]
		for i := uint(0); i < nx; i++ {
			switch bb[0] >> 4 {
			case ipv4.Version:
				l := int(bb[2])<<8 | int(bb[3])
				if m.filter.CheckIPv4(bb) {
					if _, ew := w.Write(bb[:l]); ew != nil {
						err = ew
						break
					}
					// set address flag to NoChecksum to avoid calculate checksum
					addr[i].Flags |= flags
					// set TTL to 0
					bb[8] = 0
				}
				bb = bb[l:]
			case ipv6.Version:
				l := int(bb[4])<<8 | int(bb[5]) + ipv6.HeaderLen
				if m.filter.CheckIPv6(bb) {
					if _, ew := w.Write(bb[:l]); ew != nil {
						err = ew
						break
					}
					// set address flag to NoChecksum to avoid calculate checksum
					addr[i].Flags |= flags
					// set TTL to 0
					bb[7] = 0
				}
				bb = bb[l:]
			default:
				err = errors.New("invalid ip version")
				break
			}
		}

		m.h.Lock()
		_, ew := m.h.SendEx(buff[:nb], addr[:nx])
		m.h.Unlock()
		if ew == nil || errors.Is(ew, windivert.ErrHostUnreachable) {
			continue
		}
		err = ew
		break
	}
	if err != nil {
		select {
		case <-m.closedCh:
			err = nil
		default:
		}
	}
	return
}

// OpenDevice ...
func OpenDevice(ifname, filter string, allowApps []string) (dev *Device, err error) {
	ifIdx, subIfIdx, err := getDefaultInterface(ifname)
	if err != nil {
		logx.E("[divert] %v", err)
		return nil, err
	}

	filter = fmt.Sprintf("ifIdx = %d and %s", ifIdx, filter)
	logx.D("[divert] OpenDevice() apps: %v, filter: %s", allowApps, filter)

	// h, err := windivert.Open(filter, windivert.LayerNetwork, 0, 0 /*windivert.FlagSniff|windivert.FlagRecvOnly*/)
	h, err := windivert.Open(filter, windivert.LayerNetwork, windivert.PriorityDefault, windivert.FlagDefault)
	if err != nil {
		logx.E("[divert] %v", err)
		return
	}
	defer func() {
		if err != nil {
			h.Close()
		}
	}()

	if err = h.SetParam(windivert.QueueLength, windivert.QueueLengthMax); err != nil {
		logx.E("[divert] %v", err)
		return
	}

	if err = h.SetParam(windivert.QueueSize, windivert.QueueSizeMax); err != nil {
		logx.E("[divert] %v", err)
		return
	}

	if err = h.SetParam(windivert.QueueTime, windivert.QueueTimeMax); err != nil {
		logx.E("[divert] %v", err)
		return
	}

	dev = &Device{
		addr:     new(windivert.Address),
		h:        h,
		filter:   NewPacketFilter(),
		closedCh: make(chan struct{}),
	}

	dev.p.PipeReader, dev.p.PipeWriter = io.Pipe()
	dev.p.eventCh = make(chan struct{}, 1)

	nw := dev.addr.Network()
	nw.InterfaceIndex = ifIdx
	nw.SubInterfaceIndex = subIfIdx

	// allow these .exe
	for _, app := range allowApps {
		dev.filter.AppFilter.Add(app)
	}

	go dev.loop()

	return
}

// getDefaultInterface is ...
func getDefaultInterface(ifname string) (uint32, uint32, error) {
	const FILTER = "not loopback and outbound and tcp.DstPort = 53 and (ip.DstAddr = 8.8.8.8 or ipv6.DstAddr = 2001:4860:4860::8888)"

	filter, err := func() (string, error) {
		if len(ifname) > 0 {
			ifc, err := net.InterfaceByName(ifname)
			if err != nil {
				return "", err
			}

			return fmt.Sprintf("ifIdx = %d and %s", ifc.Index, FILTER), nil
		}
		return FILTER, nil
	}()
	if err != nil {
		return 0, 0, err
	}

	// logx.D("[divert] getDefaultInterface() filter: %s", filter)

	hd, err := windivert.Open(filter, windivert.LayerNetwork, windivert.PriorityDefault, windivert.FlagSniff)
	if err != nil {
		return 0, 0, fmt.Errorf("windivert.Open error: %w", err)
	}
	defer hd.Close()

	wg := &sync.WaitGroup{}

	wg.Add(1)
	go func(wg *sync.WaitGroup) {
		defer wg.Done()

		conn, err := net.DialTimeout("tcp4", "8.8.8.8:53", time.Second)
		if err != nil {
			return
		}

		conn.Close()
	}(wg)

	wg.Add(1)
	go func(wg *sync.WaitGroup) {
		defer wg.Done()

		conn, err := net.DialTimeout("tcp6", "[2001:4860:4860::8888]:53", time.Second)
		if err != nil {
			return
		}

		conn.Close()
	}(wg)

	addr := windivert.Address{}
	buff := make([]byte, 1500)

	if _, err := hd.Recv(buff, &addr); err != nil {
		return 0, 0, err
	}

	if err := hd.Shutdown(windivert.ShutdownBoth); err != nil {
		return 0, 0, fmt.Errorf("shutdown interface handle error: %w", err)
	}

	if err := hd.Close(); err != nil {
		return 0, 0, fmt.Errorf("close interface handle error: %w", err)
	}

	wg.Wait()

	nw := addr.Network()
	return nw.InterfaceIndex, nw.SubInterfaceIndex, nil
}
