//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/io/io.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "flow.h"

namespace nx {
namespace gopacket {
using namespace gx;

// LayerType is a unique identifier for each type of layer.  This enumeration
// does not match with any externally available numbering scheme... it's solely
// usable/useful within this library as a means for requesting layer types
// (see Packet.Layer) and determining which types of layers have been decoded.
//
// New LayerTypes may be created by calling gopacket.RegisterLayerType.
using LayerType = int64;

// Layer represents a single decoded packet layer (using either the
// OSI or TCP/IP definition of a layer).  When decoding, a packet's data is
// broken up into a number of layers.  The caller may call LayerType() to
// figure out which type of layer they've received from the packet.  Optionally,
// they may then use a type assertion to get the actual layer type for deep
// inspection of the data.
struct Layer {
    // LayerType is the gopacket type for this layer.
    virtual LayerType LayerType() = 0;
    // LayerContents returns the set of bytes that make up this layer.
    virtual bytez<> LayerContents() = 0;
    // LayerPayload returns the set of bytes contained within this layer, not
    // including the layer itself.
    virtual bytez<> LayerPayload() = 0;
};

// Payload is a Layer containing the payload of a packet.  The definition of
// what constitutes the payload of a packet depends on previous layers; for
// TCP and UDP, we stop decoding above layer 4 and return the remaining
// bytes as a Payload.  Payload is an ApplicationLayer.
struct Payload {
    // NextLayerType implements DecodingLayer.
    LayerType NextLayerType();

    // LayerType returns LayerTypePayload
    LayerType LayerType();

    // LayerContents returns the bytes making up this layer.
    bytez<> LayerContents();

    // LayerPayload returns the payload within this layer.
    bytez<> LayerPayload();

    // Payload returns this layer as bytes.
    bytez<> Payloads();

    // String implements fmt.Stringer.
    string String();
};

// Fragment is a Layer containing a fragment of a larger frame, used by layers
// like IPv4 and IPv6 that allow for fragmentation of their payloads.
struct Fragment {
    // NextLayerType implements DecodingLayer.
    LayerType NextLayerType();
        // LayerType returns LayerTypePayload
    LayerType LayerType();

    // LayerContents implements Layer.
    bytez<> LayerContents();

    // LayerPayload implements Layer.
    bytez<> LayerPayload();

    // Payload returns this layer as a byte slice.
    bytez<> Payloads();

    // String implements fmt.Stringer.
    string String();

};

// LinkLayer is the packet layer corresponding to TCP/IP layer 1 (OSI layer 2)
struct LinkLayer : public Layer  {
	virtual Flow LinkFlow()  = 0;
};

// NetworkLayer is the packet layer corresponding to TCP/IP layer 2 (OSI
// layer 3)
struct NetworkLayer : public Layer {
	virtual Flow NetworkFlow() = 0;
};

// TransportLayer is the packet layer corresponding to the TCP/IP layer 3 (OSI
// layer 4)
struct TransportLayer : public Layer {
	virtual Flow TransportFlow() = 0;
};

// ApplicationLayer is the packet layer corresponding to the TCP/IP layer 4 (OSI
// layer 7), also known as the packet payload.
struct ApplicationLayer : public Layer {
	virtual bytez<> Payload() = 0;
};

// ErrorLayer is a packet layer created when decoding of the packet has failed.
// Its payload is all the bytes that we were unable to decode, and the returned
// error details why the decoding failed.
struct ErrorLayer : public Layer {
	virtual error Error()  = 0;
};


}  // namespace gopacket
}  // namespace nx
