//
// weproxy@foxmail.com 2022/11/01
//

package aescfb

import (
	"crypto/aes"
	"crypto/cipher"
	"crypto/rand"
	"errors"
	"io"
	"net"
)

//////////////////////////////////////////////////////////////////////////////////////////////////

// ErrShortPacket ...
var ErrShortPacket = errors.New("short packet")

// pack ...
func pack(dst, pkt []byte, key Cipher) ([]byte, error) {
	iv := dst[:aes.BlockSize]
	if _, err := io.ReadFull(rand.Reader, iv); err != nil {
		return nil, err
	}

	block, err := aes.NewCipher([]byte(key))
	if err != nil {
		return nil, err
	}

	encrypted := dst[aes.BlockSize : aes.BlockSize+len(pkt)]

	stream := cipher.NewCFBEncrypter(block, iv)
	stream.XORKeyStream(encrypted, pkt)

	return dst[:aes.BlockSize+len(pkt)], nil
}

// unpack ...
func unpack(dst, pkt []byte, key Cipher) ([]byte, error) {
	if len(pkt) < aes.BlockSize {
		return nil, errors.New("ciphertext too short")
	}

	iv := pkt[:aes.BlockSize]

	block, err := aes.NewCipher([]byte(key))
	if err != nil {
		return nil, err
	}

	encrypted := pkt[aes.BlockSize:]

	stream := cipher.NewCFBDecrypter(block, iv)
	stream.XORKeyStream(dst, encrypted)

	return dst[:len(encrypted)], nil
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// packetConn ...
type packetConn struct {
	net.PacketConn
	Cipher  Cipher
	isLocal bool
}

// NewPacketConn ...
func NewPacketConn(pc net.PacketConn, ciph Cipher, isLocal bool) net.PacketConn {
	return &packetConn{PacketConn: pc, Cipher: ciph, isLocal: isLocal}
}

// ReadFrom ...
func (m *packetConn) ReadFrom(b []byte) (int, net.Addr, error) {
	buf := make([]byte, len(b)+aes.BlockSize)

	n, addr, err := m.PacketConn.ReadFrom(buf)
	if err != nil {
		return 0, nil, err
	}

	var bb []byte
	if m.isLocal {
		bb, err = pack(b, buf[:n], m.Cipher)
	} else {
		bb, err = unpack(b, buf[:n], m.Cipher)
	}
	if err != nil {
		return 0, nil, err
	}

	return len(bb), addr, nil
}

// WriteTo ...
func (m *packetConn) WriteTo(b []byte, addr net.Addr) (n int, err error) {
	buf := make([]byte, len(b)+aes.BlockSize)

	var bb []byte
	if m.isLocal {
		bb, err = unpack(buf, b, m.Cipher)
	} else {
		bb, err = pack(buf, b, m.Cipher)
	}
	if err != nil {
		return 0, err
	}

	_, err = m.PacketConn.WriteTo(bb, addr)
	return len(b), err
}
