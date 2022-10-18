//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/io/io.h"
#include "gx/net/net.h"
#include "gx/time/time.h"

namespace nx {
namespace gopacket {
using namespace gx;

// MaxEndpointSize determines the maximum size in bytes of an endpoint address.
//
// Endpoints/Flows have a problem:  They need to be hashable.  Therefore, they
// can't use a byte slice.  The two obvious choices are to use a string or a
// byte array.  Strings work great, but string creation requires memory
// allocation, which can be slow.  Arrays work great, but have a fixed size.  We
// originally used the former, now we've switched to the latter.  Use of a fixed
// byte-array doubles the speed of constructing a flow (due to not needing to
// allocate).  This is a huge increase... too much for us to pass up.
//
// The end result of this, though, is that an endpoint/flow can't be created
// using more than MaxEndpointSize bytes per address.
const int MaxEndpointSize = 16;

// EndpointType is the type of a gopacket Endpoint.  This type determines how
// the bytes stored in the endpoint should be interpreted.
using EndpointType = int64;

// Endpoint is the set of bytes used to address packets at various layers.
// See LinkLayer, NetworkLayer, and TransportLayer specifications.
// Endpoints are usable as map keys.
struct endpoint_t {
    EndpointType typ;
    int len;
    byte raw[MaxEndpointSize];
};

// Endpoint ...
using Endpoint = Ref<endpoint_t>;

// Flow represents the direction of traffic for a packet layer, as a source and destination Endpoint.
// Flows are usable as map keys.
struct flow_t {
    EndpointType typ;
    int slen, dlen;
    byte src[MaxEndpointSize], dst[MaxEndpointSize];
};

// Flow ...
using Flow = Ref<flow_t>;

}  // namespace gopacket
}  // namespace nx
