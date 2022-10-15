//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../nx.h"
#include "gx/net/net.h"
#include "gx/x/time/rate/rate.h"

namespace nx {
namespace netio {

////////////////////////////////////////////////////////////////////////////////////////////////////
// Packet ...
struct Packet {
    net::Addr Addr;
    slice<byte> Data;
};

// CopingFn ...
typedef std::function<void(int)> CopingFn;

// CopyOption ...
struct CopyOption {
    rate::Limiter Limit;
    CopingFn CopingFn;
};

// RelayOption ...
struct RelayOption {
    CopyOption Read;
    CopyOption Write;
    Packet ToWrite;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copy ...
R<size_t /*w*/, error> Copy(net::Conn w, net::Conn r, CopyOption opt = {});
R<size_t /*w*/, error> Copy(net::PacketConn w, net::PacketConn r, CopyOption opt = {});

////////////////////////////////////////////////////////////////////////////////////////////////////
// Relay ...
R<size_t /*r*/, size_t /*w*/, error> Relay(net::Conn c, net::Conn rc, RelayOption opt = {});
R<size_t /*r*/, size_t /*w*/, error> Relay(net::PacketConn c, net::PacketConn rc, RelayOption opt = {});

}  // namespace netio
}  // namespace nx
