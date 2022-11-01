//
// weproxy@foxmail.com 2022/11/01
//

package aescfb

import (
	"crypto/md5"
	"fmt"
	"io"
	"net"
	"strings"
)

//////////////////////////////////////////////////////////////////////////////////////////////////

// Cipher ...
type Cipher []byte

func (m Cipher) NewReadeCloser(r io.ReadCloser) io.ReadCloser   { return NewReader(r, m) }
func (m Cipher) NewWriteCloser(w io.WriteCloser) io.WriteCloser { return NewWriter(w, m) }
func (m Cipher) NewPacketConn(c net.PacketConn, isLocal bool) net.PacketConn {
	return NewPacketConn(c, m, isLocal)
}
func (m Cipher) NewConn(c net.Conn) net.Conn { return NewConn(c, m) }

//////////////////////////////////////////////////////////////////////////////////////////////////

// CipherMatched ..
func CipherMatched(cipher string) bool {
	switch strings.ToUpper(cipher) {
	case "AES-256-CFB", "AES_256_CFB",
		"AES-CFB-256", "AES_CFB_256",
		"AES256CFB", "AESCFB256":
		return true
	default:
		return false
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// NewCipher ...
func NewCipher(cipher, password string) (Cipher, error) {
	if CipherMatched(cipher) {
		return Cipher([]byte(kdf(password, 32))), nil
	}

	return nil, fmt.Errorf("not support cipher type: %v", cipher)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

func kdf(password string, keyLen int) []byte {
	var b, prev []byte
	h := md5.New()
	for len(b) < keyLen {
		h.Write(prev)
		h.Write([]byte(password))
		b = h.Sum(b)
		prev = b[len(b)-h.Size():]
		h.Reset()
	}
	return b[:keyLen]
}
