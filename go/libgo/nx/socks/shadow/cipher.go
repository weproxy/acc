//
// weproxy@foxmail.com 2022/11/01
//

package shadow

import (
	"fmt"
	"io"
	"net"

	"weproxy/acc/libgo/nx/socks/shadow/aescfb"
	// "weproxy/acc/libgo/nx/socks/shadow/aesgcm"
)

// Cipher ...
type Cipher interface {
	NewReadCloser(io.ReadCloser) io.ReadCloser
	NewWriteCloser(io.WriteCloser) io.WriteCloser
	NewPacketConn(pc net.PacketConn, isLocal bool) net.PacketConn
	NewConn(conn net.Conn) net.Conn
}

// NewCipher ...
func NewCipher(cipher, password string) (Cipher, error) {
	if aescfb.Match(cipher) {
		return aescfb.NewCipher(cipher, password)
		// } else if aesgcm.Match(cipher) {
		// 	return aesgcm.NewCipher(cipher, password)
	}

	return nil, fmt.Errorf("invalid cipher: %v", cipher)
}
