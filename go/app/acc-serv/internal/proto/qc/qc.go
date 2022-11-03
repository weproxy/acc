//
// weproxy@foxmail.com 2022/10/20
//

package qc

import (
	"context"
	"crypto/rand"
	"crypto/rsa"
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"encoding/pem"
	"errors"
	"io"
	"math/big"
	"net"

	"github.com/lucas-clemente/quic-go"

	"weproxy/acc/libgo/logx"
	"weproxy/acc/libgo/nx/socks"

	"weproxy/acc/app/acc-serv/internal/proto"
)

// TAG ...
const TAG = "[qc]"

// init ...
func init() {
	proto.Register([]string{"qc", "quic"}, New)
}

// New ...
func New(js json.RawMessage) (proto.Server, error) {
	var j struct {
		Listen string `json:"listen,omitempty"`
	}

	if err := json.Unmarshal(js, &j); err != nil {
		return nil, err
	}
	if len(j.Listen) == 0 {
		return nil, errors.New("missing addr")
	}

	return &server_t{addr: j.Listen}, nil
}

// server_t ...
type server_t struct {
	addr string
	ln   quic.Listener
}

// Start ...
func (m *server_t) Start() error {
	tlsCfg, err := generateTLSConfig()
	if err != nil {
		logx.E("%v Listen(%v), err: %v", TAG, m.addr, err)
		return err
	}

	ln, err := quic.ListenAddr(m.addr, tlsCfg, nil)
	if err != nil {
		logx.E("%v Listen(%v), err: %v", TAG, m.addr, err)
		return err
	}

	logx.D("%v Start(%s)", TAG, m.addr)

	m.ln = ln
	go func() {
		ctx := context.Background()
		for {
			c, err := ln.Accept(ctx)
			if err != nil {
				if !errors.Is(err, quic.ErrServerClosed) {
					logx.E("%v Accept(), err: %v", TAG, err)
				}
				break
			}

			logx.V("%v Accept() %v", TAG, c.RemoteAddr())

			go m.handleConn(ctx, c)
		}
	}()

	return nil
}

// Close ...
func (m *server_t) Close() error {
	if m.ln != nil {
		m.ln.Close()
	}
	logx.D("%v Close()", TAG)
	return nil
}

////////////////////////////////////////////////////////////////////////////////

// quicConn ...
type quicConn struct {
	quic.Stream
	c quic.Connection
}

// LocalAddr/RemoteAddr ...
func (m *quicConn) LocalAddr() net.Addr  { return m.c.LocalAddr() }
func (m *quicConn) RemoteAddr() net.Addr { return m.c.RemoteAddr() }

// generateTLSConfig ...
func generateTLSConfig() (tlsCfg *tls.Config, err error) {
	key, err := rsa.GenerateKey(rand.Reader, 1024)
	if err != nil {
		return
	}

	template := x509.Certificate{SerialNumber: big.NewInt(1)}

	certDER, err := x509.CreateCertificate(rand.Reader, &template, &template, &key.PublicKey, key)
	if err != nil {
		return
	}

	keyPEM := pem.EncodeToMemory(&pem.Block{Type: "RSA PRIVATE KEY", Bytes: x509.MarshalPKCS1PrivateKey(key)})
	certPEM := pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: certDER})

	tlsCert, err := tls.X509KeyPair(certPEM, keyPEM)
	if err != nil {
		return
	}

	tlsCfg = &tls.Config{
		Certificates: []tls.Certificate{tlsCert},
		NextProtos:   []string{"weproxy-quic"}, // don't edit it, this string must same as client
	}

	return
}

////////////////////////////////////////////////////////////////////////////////

// handleConn ...
func (m *server_t) handleConn(ctx context.Context, qc quic.Connection) {
	s, err := qc.AcceptStream(ctx)
	if err != nil {
		return
	}

	c := &quicConn{c: qc, Stream: s}
	defer c.Close()

	var buf [4]byte

	// >>> REQ:
	//
	//	| HEAD | ATYP | DST.ADDR | DST.PORT |
	//	+------+------+----------+----------+
	//	|  4   |   1  |    ...   |    2     |

	_, err = io.ReadFull(c, buf[:])
	if err != nil {
		logx.E("%v handleConn(), ReadHead err: %v", TAG, err)
		return
	}

	_, raddr, err := socks.ReadAddr(c)
	if err != nil {
		logx.E("%v handleConn(), ReadAddr err: %v", TAG, err)
		return
	}

	if buf[0]&0x01 != 0 {
		go m.handleTCP(c, raddr.ToTCPAddr())
	} else {
		go m.handleUDP(c, raddr.ToUDPAddr())
	}
}
