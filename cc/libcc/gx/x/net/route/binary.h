//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/errors/errors.h"
#include "gx/fmt/fmt.h"

namespace gx {
namespace x {
namespace net {
namespace route {

// This file contains duplicates of encoding/binary package.
//
// This package is supposed to be used by the net package of standard
// library. Therefore the package set used in the package must be the
// same as net package.

struct binaryByteOrder {
    virtual uint16 Uint16(slice<>) = 0;
    virtual uint32 Uint32(slice<>) = 0;
    virtual void PutUint16(slice<>, uint16) = 0;
    virtual void PutUint32(slice<>, uint32) = 0;
    virtual uint64 Uint64(slice<>) = 0;
};

struct binaryLittleEndian : public binaryByteOrder {
    virtual uint16 Uint16(slice<> b) override { return (uint16(b[0]) | uint16(b[1]) << 8); }

    virtual void PutUint16(slice<> b, uint16 v) override {
        b[0] = byte(v);
        b[1] = byte(v >> 8);
    }

    virtual uint32 Uint32(slice<> b) override {
        return (uint32(b[0]) | uint32(b[1]) << 8 | uint32(b[2]) << 16 | uint32(b[3]) << 24);
    }

    virtual void PutUint32(slice<> b, uint32 v) override {
        b[0] = byte(v);
        b[1] = byte(v >> 8);
        b[2] = byte(v >> 16);
        b[3] = byte(v >> 24);
    }

    virtual uint64 Uint64(slice<> b) override {
        return (uint64(b[0]) | uint64(b[1]) << 8 | uint64(b[2]) << 16 | uint64(b[3]) << 24 | uint64(b[4]) << 32 |
                uint64(b[5]) << 40 | uint64(b[6]) << 48 | uint64(b[7]) << 56);
    }
};

struct binaryBigEndian : public binaryByteOrder {
    virtual uint16 Uint16(slice<> b) override { return (uint16(b[1]) | uint16(b[0]) << 8); }

    virtual void PutUint16(slice<> b, uint16 v) override {
        b[0] = byte(v >> 8);
        b[1] = byte(v);
    }

    virtual uint32 Uint32(slice<> b) override {
        return (uint32(b[3]) | uint32(b[2]) << 8 | uint32(b[1]) << 16 | uint32(b[0]) << 24);
    }

    virtual void PutUint32(slice<> b, uint32 v) override {
        b[0] = byte(v >> 24);
        b[1] = byte(v >> 16);
        b[2] = byte(v >> 8);
        b[3] = byte(v);
    }

    virtual uint64 Uint64(slice<> b) override {
        return (uint64(b[7]) | uint64(b[6]) << 8 | uint64(b[5]) << 16 | uint64(b[4]) << 24 | uint64(b[3]) << 32 |
                uint64(b[2]) << 40 | uint64(b[1]) << 48 | uint64(b[0]) << 56);
    }
};

////////////////////////////////////////////////////////////////////////////////
//
extern binaryLittleEndian littleEndian;
extern binaryBigEndian bigEndian;

}  // namespace route
}  // namespace net
}  // namespace x
}  // namespace gx
