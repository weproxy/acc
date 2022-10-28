//
// weproxy@foxmail.com 2022/10/20
//

package eth

import (
	"errors"
	"net"
	"sync"
	"syscall"
	"time"

	"github.com/google/gopacket"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/device"
	"weproxy/acc/libgo/nx/device/eth"
	"weproxy/acc/libgo/nx/stack/netstk"
	"weproxy/acc/libgo/nx/util"
)

// init ...
func init() {
	device.Register(device.TypeRAW, New)
}

// New ...
func New(cfg device.Conf) (netstk.Device, error) {
	ifname, cidr := cfg.Str("ifname"), cfg.Str("cidr")
	if len(ifname) == 0 || len(cidr) == 0 {
		return nil, errors.New("missing cfg ifname/cidr")
	}

	newInnerFn := func(ifc *net.Interface, cidr net.IPNet) (eth.Inner, error) {
		if err := util.SetPromiscMode(ifname, true); err != nil {
			logx.E("[raw] SetPromiscMode(%s) fail: %v", ifname, err)
			return nil, err
		}

		inner := &rawDevice{
			ifIdx:  ifc.Index,
			ifName: ifc.Name,
			tmpBuf: make([]byte, 2048),
		}
		if err := inner.Open(ifc, cidr); err != nil {
			return nil, err
		}
		return inner, nil
	}

	return eth.New(ifname, cidr, newInnerFn)
}

////////////////////////////////////////////////////////////////////////////////

// rawDevice implements eth.Inner
type rawDevice struct {
	fd     int
	ifIdx  int
	ifName string
	toAddr syscall.SockaddrLinklayer // Sendto(...&sa)
	tmpBuf []byte
	mu     sync.Mutex
}

// Type ...
func (m *rawDevice) Type() string {
	return device.TypeRAW
}

// Close implements io.Closer
func (m *rawDevice) Close() error {
	if m.fd > 0 {
		syscall.Close(m.fd)
	}
	if len(m.ifName) > 0 {
		util.SetPromiscMode(m.ifName, false)
	}
	return nil
}

// Open ...
func (m *rawDevice) Open(ifc *net.Interface, cidr net.IPNet) (err error) {
	m.toAddr = func(ifc *net.Interface) syscall.SockaddrLinklayer {
		var haddr [8]byte
		copy(haddr[0:7], ifc.HardwareAddr[0:7])
		return syscall.SockaddrLinklayer{
			// Protocol: syscall.ETH_P_IP,
			Ifindex: ifc.Index,
			Halen:   uint8(len(ifc.HardwareAddr)),
			Addr:    haddr,
		}
	}(ifc)

	m.fd, err = util.CreateRawSocket(true)
	if err != nil {
		logx.E("[raw] util.CreateRawSocket() %v", err)
		return
	}

	return
}

// ReadPacketData for gopacket interface
func (m *rawDevice) ReadPacketData() (data []byte, ci gopacket.CaptureInfo, err error) {
	// logx.D("[netdev] raw.ReadPacketData...")

	m.mu.Lock()
	defer m.mu.Unlock()

	n, _, err := syscall.Recvfrom(m.fd, m.tmpBuf, 0)
	if err != nil {
		// logx.E("[netdev] raw,ReadPacketData, %v", err)
		return
	}

	ci.Timestamp = time.Now()
	ci.Length = n
	ci.CaptureLength = n
	ci.InterfaceIndex = m.ifIdx

	data = make([]byte, ci.CaptureLength)
	n = copy(data, m.tmpBuf[:n])

	// logx.D("[netdev] raw.ReadPacketData, %v bytes", n)

	return
}

// WritePacketData ...
func (m *rawDevice) WritePacketData(data []byte) (err error) {
	err = syscall.Sendto(m.fd, data, 0, &m.toAddr)
	if err != nil {
		// logx.E("[netdev] raw,WritePacketData, %v", err)
	} else {
		// logx.D("[netdev] raw.WritePacketData, %v bytes", len(data))
	}
	return
}
