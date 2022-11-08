//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/io/io.h"
#include "gx/net/net.h"

namespace nx {
namespace stack {
using namespace gx;

////////////////////////////////////////////////////////////////////////////////
// handler_t ...
struct handler_t : public io::xx::closer_t {
    virtual void Handle(net::Conn c, net::Addr raddr) = 0;
    virtual void HandlePacket(net::PacketConn pc, net::Addr raddr) = 0;
    virtual error Close() override = 0;
};

using Handler = Ref<handler_t>;

// SetHandler ...
void SetHandler(Handler h);

// ...
}  // namespace stack
}  // namespace nx
