//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../nx.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "gx/x/time/rate/rate.h"

namespace nx {
namespace netio {

////////////////////////////////////////////////////////////////////////////////
// Packet ...
struct Packet {
    net::Addr Addr;
    bytez<> Data;
};

// CopingFn ...
using CopingFn = func<void(int)>;

// CopyOption ...
struct CopyOption {
    Ref<rate::Limiter> Limit;
    CopingFn CopingFn;
    time::Duration ReadTimeout;
    time::Duration WriteTimeout;
    net::Addr WriteAddr;
    int MaxTimes{0}; // max copy times

    CopyOption() = default;
    CopyOption(time::Duration timeout) : ReadTimeout(timeout), WriteTimeout(timeout) {}
    CopyOption(time::Duration readTimeout, time::Duration writeTimeout)
        : ReadTimeout(readTimeout), WriteTimeout(writeTimeout) {}
};

// RelayOption ...
struct RelayOption {
    CopyOption A2B;
    CopyOption B2A;
    Packet ToB;

    RelayOption() = default;
    RelayOption(time::Duration timeout) : A2B(timeout), B2A(timeout) {}
    RelayOption(time::Duration readTimeout, time::Duration writeTimeout)
        : A2B(readTimeout, writeTimeout), B2A(readTimeout, writeTimeout) {}
};

////////////////////////////////////////////////////////////////////////////////
// Copy ...
R<size_t /*w*/, error> Copy(net::Conn w, net::Conn r, CopyOption opt = {});
R<size_t /*w*/, error> Copy(net::PacketConn w, net::PacketConn r, CopyOption opt = {});

////////////////////////////////////////////////////////////////////////////////
// Relay ...
error Relay(net::Conn a, net::Conn b, RelayOption opt = {});
error Relay(net::PacketConn a, net::PacketConn b, RelayOption opt = {});

}  // namespace netio
}  // namespace nx
