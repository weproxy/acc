//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/encoding/binary/binary.h"
#include "gx/net/net.h"

namespace nx {
using namespace gx;

// NewID ...
uint64 NewID();

// BindInterface ...
error BindInterface(int fd, int ifaceInex);

////////////////////////////////////////////////////////////////////////////////
// Key ...
using Key = uint64;

// MakeKey ...
inline Key MakeKey(net::IP ip, uint16 port) {
    uint64 a = (uint64)binary::LittleEndian.Uint32(ip.B);  // ipv4
    uint64 b = (uint64)port;
    return a << 16 | b;
}

// MakeKey ...
inline Key MakeKey(net::Addr addr) { return MakeKey(addr->IP, addr->Port); }

}  // namespace nx
