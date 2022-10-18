//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "base.h"
#include "gx/io/io.h"
#include "gx/net/net.h"
#include "gx/time/time.h"

namespace nx {
namespace gopacket {
using namespace gx;

// CaptureInfo provides standardized information about a packet captured off
// the wire or read from a file.
struct CaptureInfo {
    // Timestamp is the time the packet was captured, if that is known.
    time::Time Timestamp;
    // CaptureLength is the total number of bytes read off of the wire.
    int CaptureLength{0};
    // Length is the size of the original packet.  Should always be >=
    // CaptureLength.
    int Length{0};
    // InterfaceIndex
    int InterfaceIndex{-1};
    // The packet source can place ancillary data of various types here.
    // For example, the afpacket source can report the VLAN of captured
    // packets this way.
    slice<void*> AncillaryData;
};

// PacketMetadata contains metadata for a packet.
struct PacketMetadata : public CaptureInfo {
    // Truncated is true if packet decoding logic detects that there are fewer
    // bytes in the packet than are detailed in various headers (for example, if
    // the number of bytes in the IPv4 contents/payload is less than IPv4.Length).
    // This is also set automatically for packets captured off the wire if
    // CaptureInfo.CaptureLength < CaptureInfo.Length.
    bool Truncated{false};
};

// packet contains all the information we need to fulfill the Packet interface,
// and its two "subclasses" (yes, no such thing in Go, bear with me),
// eagerPacket and lazyPacket, provide eager and lazy decoding logic around the
// various functions needed to access this information.
struct packet {
    // data contains the entire packet data for a packet
    slice<byte> data;
    // initialLayers is space for an initial set of layers already created inside
    // the packet.
    slice<Layer> initialLayers;  // 6
    // layers contains each layer we've already decoded
    slice<Layer> layers;
    // last is the last layer added to the packet
    Layer* last;
    // metadata is the PacketMetadata for this packet
    PacketMetadata metadata;

    // DecodeOptions decodeOptions;

    // // Pointers to the various important layers
    // LinkLayer link;
    // NetworkLayer network;
    // TransportLayer transport;
    // ApplicationLayer application;
    // ErrorLayer failure;
};

}  // namespace gopacket
}  // namespace nx
