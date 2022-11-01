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

// CloseReader ...
type CloseReader interface {
	CloseRead() error
}

// CloseWriter ...
type CloseWriter interface {
	CloseWrite() error
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// Reader ...
type Reader struct {
	Reader io.ReadCloser
	Cipher Cipher
	stream cipher.Stream

	buff []byte
	left []byte
}

// NewReader ...
func NewReader(r io.ReadCloser, cipher Cipher) *Reader {
	return &Reader{
		Reader: r,
		Cipher: cipher,
	}
}

// init ...
func (m *Reader) init() (err error) {
	// read IV in first packet
	iv := make([]byte, aes.BlockSize)
	if _, err := io.ReadFull(m.Reader, iv); err != nil {
		return err
	}

	block, err := aes.NewCipher([]byte(m.Cipher))
	if err != nil {
		return err
	}
	// decrypter
	m.stream = cipher.NewCFBDecrypter(block, iv)

	m.buff = make([]byte, 32*1024)
	return
}

// Close ...
func (m *Reader) Close() (err error) {
	if closer, ok := m.Reader.(CloseReader); ok {
		err = closer.CloseRead()
		return
	}
	return m.Reader.Close()
}

// read ...
func (m *Reader) read() (int, error) {
	// read from
	n, err := m.Reader.Read(m.buff)
	if err != nil {
		return 0, err
	}

	// decrypt
	m.stream.XORKeyStream(m.buff[:n], m.buff[:n])

	return n, nil
}

// Read ...
func (m *Reader) Read(b []byte) (int, error) {
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
func (m *Reader) WriteTo(w io.Writer) (n int64, err error) {
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

// Writer ...
type Writer struct {
	Writer io.WriteCloser
	Cipher Cipher
	stream cipher.Stream

	reader *bytes.Reader

	buff []byte
}

// NewWriter ...
func NewWriter(w io.WriteCloser, cipher Cipher) *Writer {
	return &Writer{
		Writer: w,
		Cipher: cipher,
	}
}

// init ...
func (m *Writer) init() error {
	// create IV
	iv := make([]byte, aes.BlockSize)
	_, err := rand.Read(iv)
	if err != nil {
		return err
	}

	block, err := aes.NewCipher([]byte(m.Cipher))
	if err != nil {
		return err
	}
	// Encrypter
	m.stream = cipher.NewCFBEncrypter(block, iv)

	m.reader = bytes.NewReader(nil)

	m.buff = make([]byte, 32*1024)

	// first packet write IV
	_, err = m.Writer.Write(iv)
	return err
}

// Close ...
func (m *Writer) Close() (err error) {
	if closer, ok := m.Writer.(CloseWriter); ok {
		err = closer.CloseWrite()
		return
	}
	return m.Writer.Close()
}

// Write ...
func (m *Writer) Write(b []byte) (int, error) {
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
func (m *Writer) ReadFrom(r io.Reader) (int64, error) {
	if m.stream == nil {
		if err := m.init(); err != nil {
			return 0, err
		}
	}

	return m.readFrom(r)
}

// readFrom ...
func (m *Writer) readFrom(r io.Reader) (n int64, err error) {
	buf := make([]byte, 32*1024)

	for {
		nr, er := r.Read(buf)
		if nr > 0 {
			n += int64(nr)

			dat := buf[:nr]

			// encrypt
			m.stream.XORKeyStream(dat, dat)

			// send
			_, err = m.Writer.Write(dat)
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

// Conn ...
type Conn struct {
	net.Conn
	Reader Reader
	Writer Writer
}

// NewConn ...
func NewConn(conn net.Conn, cipher Cipher) net.Conn {
	return &Conn{
		Conn: conn,
		Reader: Reader{
			Reader: conn,
			Cipher: cipher,
		},
		Writer: Writer{
			Writer: conn,
			Cipher: cipher,
		},
	}
}

func (m *Conn) Read(b []byte) (int, error)          { return m.Reader.Read(b) }
func (m *Conn) Write(b []byte) (int, error)         { return m.Writer.Write(b) }
func (m *Conn) WriteTo(w io.Writer) (int64, error)  { return m.Reader.WriteTo(w) }
func (m *Conn) ReadFrom(r io.Reader) (int64, error) { return m.Writer.ReadFrom(r) }
func (m *Conn) CloseRead() error                    { return m.Reader.Close() }
func (m *Conn) CloseWrite() error                   { return m.Writer.Close() }
