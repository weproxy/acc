//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/io/io.h"
#include "gx/net/net.h"

namespace nx {
namespace stack {
using namespace gx;

////////////////////////////////////////////////////////////////////////////////////////////////////
// IHandler ...
struct IHandler : public io::xx::closer_t {
    virtual error Handle(net::Conn c, net::Addr raddr) = 0;
    virtual error HandlePacket(net::PacketConn c, net::Addr raddr) = 0;
    virtual void Close() = 0;
};

// Handler ...
typedef std::shared_ptr<IHandler> Handler;

// ...
}  // namespace stack
}  // namespace nx
