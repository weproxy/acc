//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "varint.h"

namespace gx {
namespace binary {

// A ByteOrder specifies how to convert byte slices into
// 16-, 32-, or 64-bit unsigned integers.
struct ByteOrder {
    virtual uint16 Uint16(bytez<>) = 0;
    virtual uint32 Uint32(bytez<>) = 0;
    virtual uint64 Uint64(bytez<>) = 0;
    virtual void PutUint16(bytez<>, uint16) = 0;
    virtual void PutUint32(bytez<>, uint32) = 0;
    virtual void PutUint64(bytez<>, uint64) = 0;

    virtual string String() const { return "ByteOrder"; }
};

// AppendByteOrder specifies how to append 16-, 32-, or 64-bit unsigned integers
// into a byte slice.
struct AppendByteOrder {
    virtual bytez<> AppendUint16(bytez<>, uint16) = 0;
    virtual bytez<> AppendUint32(bytez<>, uint32) = 0;
    virtual bytez<> AppendUint64(bytez<>, uint64) = 0;
    virtual string String() const { return "AppendByteOrder"; }
};

// littleEndian ...
struct littleEndian : public ByteOrder, public AppendByteOrder {
    // String ...
    virtual string String() const override { return "binary::LittleEndian"; }

    // uint16
    virtual uint16 Uint16(bytez<> b) override { return (uint16(b[0]) | uint16(b[1]) << 8); }

    virtual void PutUint16(bytez<> b, uint16 v) override {
        b[0] = byte(v);
        b[1] = byte(v >> 8);
    }

    virtual bytez<> AppendUint16(bytez<> b, uint16 v) override { return append(b, byte(v), byte(v >> 8)); }

    // uint32
    virtual uint32 Uint32(bytez<> b) override {
        return (uint32(b[0]) | uint32(b[1]) << 8 | uint32(b[2]) << 16 | uint32(b[3]) << 24);
    }

    virtual void PutUint32(bytez<> b, uint32 v) override {
        b[0] = byte(v);
        b[1] = byte(v >> 8);
        b[2] = byte(v >> 16);
        b[3] = byte(v >> 24);
    }

    virtual bytez<> AppendUint32(bytez<> b, uint32 v) override {
        return append(b, byte(v), byte(v >> 8), byte(v >> 16), byte(v >> 24));
    }

    // uint64
    virtual uint64 Uint64(bytez<> b) override {
        return (uint64(b[0]) | uint64(b[1]) << 8 | uint64(b[2]) << 16 | uint64(b[3]) << 24 | uint64(b[4]) << 32 |
                uint64(b[5]) << 40 | uint64(b[6]) << 48 | uint64(b[7]) << 56);
    }

    virtual void PutUint64(bytez<> b, uint64 v) override {
        b[0] = byte(v);
        b[1] = byte(v >> 8);
        b[2] = byte(v >> 16);
        b[3] = byte(v >> 24);
        b[4] = byte(v >> 32);
        b[5] = byte(v >> 40);
        b[6] = byte(v >> 48);
        b[7] = byte(v >> 56);
    }

    virtual bytez<> AppendUint64(bytez<> b, uint64 v) override {
        return append(b, byte(v), byte(v >> 8), byte(v >> 16), byte(v >> 24), byte(v >> 32), byte(v >> 40),
                      byte(v >> 48), byte(v >> 56));
    }
};

// bigEndian ...
struct bigEndian : public ByteOrder, public AppendByteOrder {
    // String ...
    virtual string String() const override { return "binary::BigEndian"; }

    // uint16
    virtual uint16 Uint16(bytez<> b) override { return (uint16(b[1]) | uint16(b[0]) << 8); }

    virtual void PutUint16(bytez<> b, uint16 v) override {
        b[0] = byte(v >> 8);
        b[1] = byte(v);
    }

    virtual bytez<> AppendUint16(bytez<> b, uint16 v) override { return append(b, byte(v >> 8), byte(v)); }

    // uint32
    virtual uint32 Uint32(bytez<> b) override {
        return (uint32(b[3]) | uint32(b[2]) << 8 | uint32(b[1]) << 16 | uint32(b[0]) << 24);
    }

    virtual void PutUint32(bytez<> b, uint32 v) override {
        b[0] = byte(v >> 24);
        b[1] = byte(v >> 16);
        b[2] = byte(v >> 8);
        b[3] = byte(v);
    }

    virtual bytez<> AppendUint32(bytez<> b, uint32 v) override {
        return append(b, byte(v >> 24), byte(v >> 16), byte(v >> 8), byte(v));
    }

    // uint64
    virtual uint64 Uint64(bytez<> b) override {
        return (uint64(b[7]) | uint64(b[6]) << 8 | uint64(b[5]) << 16 | uint64(b[4]) << 24 | uint64(b[3]) << 32 |
                uint64(b[2]) << 40 | uint64(b[1]) << 48 | uint64(b[0]) << 56);
    }

    virtual void PutUint64(bytez<> b, uint64 v) override {
        b[0] = byte(v >> 56);
        b[1] = byte(v >> 48);
        b[2] = byte(v >> 40);
        b[3] = byte(v >> 32);
        b[4] = byte(v >> 24);
        b[5] = byte(v >> 16);
        b[6] = byte(v >> 8);
        b[7] = byte(v);
    }

    virtual bytez<> AppendUint64(bytez<> b, uint64 v) override {
        return append(b, byte(v >> 56), byte(v >> 48), byte(v >> 40), byte(v >> 32), byte(v >> 24), byte(v >> 16),
                      byte(v >> 8), byte(v));
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// LittleEndian is the little-endian implementation of ByteOrder and AppendByteOrder.
extern littleEndian LittleEndian;

// BigEndian is the big-endian implementation of ByteOrder and AppendByteOrder.
extern bigEndian BigEndian;

}  // namespace binary
}  // namespace gx
