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
	NewReadeCloser(io.ReadCloser) io.ReadCloser
	NewWriteCloser(io.WriteCloser) io.WriteCloser
	NewPacketConn(pc net.PacketConn, isLocal bool) net.PacketConn
	NewConn(conn net.Conn) net.Conn
}

// CipherMatched ...
func CipherMatched(cipher string) bool {
	return aescfb.CipherMatched(cipher) /*|| aesgcm.CipherMatched(cipher)*/
}

// NewCipher ...
func NewCipher(cipher, password string) (Cipher, error) {
	if aescfb.CipherMatched(cipher) {
		return aescfb.NewCipher(cipher, password)
		// } else if aesgcm.CipherMatched(cipher) {
		// 	return aesgcm.NewCipher(cipher, password)
	}

	return nil, fmt.Errorf("invalid cipher: %v", cipher)
}
