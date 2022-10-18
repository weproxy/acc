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
    virtual error Close() override = 0;
};

// Handler ...
using Handler = Ref<IHandler>;

// ...
}  // namespace stack
}  // namespace nx
