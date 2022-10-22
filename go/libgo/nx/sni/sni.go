//
// weproxy@foxmail.com 2022/10/20
//

package sni

import (
	"bufio"
	"bytes"
	"crypto/tls"
	"errors"
	"io"
	"net"
	"net/http"
	"strings"
)

////////////////////////////////////////////////////////////////////////////////

var ErrNotTLS = errors.New("not tls")
var ErrNotHTTP = errors.New("not http")
var ErrNoServerName = errors.New("no servername")
var ErrNoExtensions = errors.New("no extensions")

////////////////////////////////////////////////////////////////////////////////

// GetServerName get servername in fist packet
//
//	TLS:  get extensions from ClientHello handshake
//	HTTP: get host from header
func GetServerName(b []byte) (serverName string, isTLS bool, err error) {
	if len(b) < 10 {
		err = io.ErrShortBuffer
		return
	}

	isTLS = b[0] == 0x16

	if isTLS {
		serverName, err = getHTTPs_v1(b)
	} else {
		serverName, err = getHTTP_v1(b)
	}

	if err == nil && strings.Contains(serverName, ":") {
		serverName, _, _ = net.SplitHostPort(serverName)
	}

	if len(serverName) == 0 {
		err = ErrNoServerName
	}

	return
}

////////////////////////////////////////////////////////////////////////////////

// getHTTP_v1
func getHTTP_v1(b []byte) (serverName string, err error) {
	br := bufio.NewReader(bytes.NewReader(b))

	req, err := http.ReadRequest(br)
	if err != nil {
		return
	}

	serverName = req.Host
	if len(serverName) == 0 {
		serverName = req.Header.Get("Host")
	}

	return
}

////////////////////////////////////////////////////////////////////////////////

// sniffConn is a net.Conn that reads from r, fails on Writes,
// and crashes otherwise.
type sniffConn struct {
	r        io.Reader
	net.Conn // nil; crash on any unexpected use
}

func (m sniffConn) Read(p []byte) (int, error)  { return m.r.Read(p) }
func (m sniffConn) Write(p []byte) (int, error) { return 0, io.EOF }

// getHTTPs_v1
func getHTTPs_v1(b []byte) (serverName string, err error) {
	br := bufio.NewReader(bytes.NewReader(b))

	const recHdrLen = 5
	hdr, err := br.Peek(recHdrLen)
	if err != nil {
		return
	}

	const recTypHandshake = 0x16
	if hdr[0] != recTypHandshake {
		err = errors.New("not tls")
		return
	}

	recLen := int(hdr[3])<<8 | int(hdr[4]) // ignoring version in hdr[1:3]
	helloBytes, err := br.Peek(recHdrLen + recLen)
	if err != nil {
		return
	}

	tls.Server(sniffConn{r: bytes.NewReader(helloBytes)}, &tls.Config{
		GetConfigForClient: func(hello *tls.ClientHelloInfo) (*tls.Config, error) {
			serverName = hello.ServerName
			return nil, nil
		},
	}).Handshake()

	return
}
