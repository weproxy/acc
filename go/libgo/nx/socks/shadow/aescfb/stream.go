//
// weproxy@foxmail.com 2022/11/01
//

package aescfb

import (
	"bytes"
	"crypto/aes"
	"crypto/cipher"
	"crypto/rand"
	"errors"
	"io"
	"net"
)

//////////////////////////////////////////////////////////////////////////////////////////////////

// closeReader ...
type closeReader interface {
	CloseRead() error
}

// closeWriter ...
type closeWriter interface {
	CloseWrite() error
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// reader ...
type reader struct {
	rd     io.ReadCloser
	cipher Cipher
	stream cipher.Stream

	buff []byte
	left []byte
}

// NewReader ...
func NewReader(r io.ReadCloser, cipher Cipher) *reader {
	return &reader{
		rd:     r,
		cipher: cipher,
	}
}

// init ...
func (m *reader) init() (err error) {
	// read IV in first packet
	iv := make([]byte, aes.BlockSize)
	if _, err := io.ReadFull(m.rd, iv); err != nil {
		return err
	}

	block, err := aes.NewCipher([]byte(m.cipher))
	if err != nil {
		return err
	}
	// decrypter
	m.stream = cipher.NewCFBDecrypter(block, iv)

	m.buff = make([]byte, 32*1024)
	return
}

// Close ...
func (m *reader) Close() (err error) {
	if closer, ok := m.rd.(closeReader); ok {
		err = closer.CloseRead()
		return
	}
	return m.rd.Close()
}

// read ...
func (m *reader) read() (int, error) {
	// read from
	n, err := m.rd.Read(m.buff)
	if err != nil {
		return 0, err
	}

	// decrypt
	m.stream.XORKeyStream(m.buff[:n], m.buff[:n])

	return n, nil
}

// Read ...
func (m *reader) Read(b []byte) (int, error) {
	if m.stream == nil {
		if err := m.init(); err != nil {
			return 0, err
		}
	}

	if len(m.left) > 0 {
		n := copy(b, m.left)
		m.left = m.left[n:]
		return n, nil
	}

	n, err := m.read()
	nr := copy(b, m.buff[:n])
	if nr < n {
		m.left = m.buff[nr:n]
	}

	return nr, err
}

// WriteTo ...
func (m *reader) WriteTo(w io.Writer) (n int64, err error) {
	if m.stream == nil {
		if err := m.init(); err != nil {
			return 0, err
		}
	}

	for len(m.left) > 0 {
		nw, ew := w.Write(m.left)
		m.left = m.left[nw:]
		n += int64(nw)
		if ew != nil {
			return n, ew
		}
	}

	for {
		nr, er := m.read()
		if nr > 0 {
			nw, ew := w.Write(m.buff[:nr])
			n += int64(nw)
			if ew != nil {
				err = ew
				break
			}
			if nr != nw {
				err = io.ErrShortWrite
				break
			}
		}
		if er != nil {
			if errors.Is(er, io.EOF) {
				break
			}
			err = er
			break
		}
	}

	return n, err
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// writer ...
type writer struct {
	wr     io.WriteCloser
	cipher Cipher
	stream cipher.Stream

	reader *bytes.Reader

	buff []byte
}

// NewWriter ...
func NewWriter(w io.WriteCloser, cipher Cipher) *writer {
	return &writer{
		wr:     w,
		cipher: cipher,
	}
}

// init ...
func (m *writer) init() error {
	// create IV
	iv := make([]byte, aes.BlockSize)
	_, err := rand.Read(iv)
	if err != nil {
		return err
	}

	block, err := aes.NewCipher([]byte(m.cipher))
	if err != nil {
		return err
	}
	// Encrypter
	m.stream = cipher.NewCFBEncrypter(block, iv)

	m.reader = bytes.NewReader(nil)

	m.buff = make([]byte, 32*1024)

	// first packet write IV
	_, err = m.wr.Write(iv)
	return err
}

// Close ...
func (m *writer) Close() (err error) {
	if closer, ok := m.wr.(closeWriter); ok {
		err = closer.CloseWrite()
		return
	}
	return m.wr.Close()
}

// Write ...
func (m *writer) Write(b []byte) (int, error) {
	if m.stream == nil {
		if err := m.init(); err != nil {
			return 0, err
		}
	}

	m.reader.Reset(b)
	n, err := m.readFrom(m.reader)
	return int(n), err
}

// ReadFrom ...
func (m *writer) ReadFrom(r io.Reader) (int64, error) {
	if m.stream == nil {
		if err := m.init(); err != nil {
			return 0, err
		}
	}

	return m.readFrom(r)
}

// readFrom ...
func (m *writer) readFrom(r io.Reader) (n int64, err error) {
	buf := make([]byte, 32*1024)

	for {
		nr, er := r.Read(buf)
		if nr > 0 {
			n += int64(nr)

			dat := buf[:nr]

			// encrypt
			m.stream.XORKeyStream(dat, dat)

			// send
			_, err = m.wr.Write(dat)
			if err != nil {
				break
			}
		}
		if er != nil {
			if errors.Is(er, io.EOF) {
				break
			}
			err = er
			break
		}
	}

	return n, err
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// conn ...
type conn struct {
	net.Conn
	rd *reader
	wr *writer
}

// NewConn ...
func NewConn(c net.Conn, cipher Cipher) net.Conn {
	return &conn{
		Conn: c,
		rd:   NewReader(c, cipher),
		wr:   NewWriter(c, cipher),
	}
}

func (m *conn) Read(b []byte) (int, error)          { return m.rd.Read(b) }
func (m *conn) Write(b []byte) (int, error)         { return m.wr.Write(b) }
func (m *conn) WriteTo(w io.Writer) (int64, error)  { return m.rd.WriteTo(w) }
func (m *conn) ReadFrom(r io.Reader) (int64, error) { return m.wr.ReadFrom(r) }
func (m *conn) CloseRead() error                    { return m.rd.Close() }
func (m *conn) CloseWrite() error                   { return m.wr.Close() }
