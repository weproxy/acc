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

struct Decoder;

// DecodeOptions tells gopacket how to decode a packet.
struct DecodeOptions {
    // Lazy decoding decodes the minimum number of layers needed to return data
    // for a packet at each function call.  Be careful using this with concurrent
    // packet processors, as each call to packet.* could mutate the packet, and
    // two concurrent function calls could interact poorly.
    bool Lazy;
    // NoCopy decoding doesn't copy its input buffer into storage that's owned by
    // the packet.  If you can guarantee that the bytes underlying the slice
    // passed into NewPacket aren't going to be modified, this can be faster.  If
    // there's any chance that those bytes WILL be changed, this will invalidate
    // your packets.
    bool NoCopy;
    // SkipDecodeRecovery skips over panic recovery during packet decoding.
    // Normally, when packets decode, if a panic occurs, that panic is captured
    // by a recover(), and a DecodeFailure layer is added to the packet detailing
    // the issue.  If this flag is set, panics are instead allowed to continue up
    // the stack.
    bool SkipDecodeRecovery;
    // DecodeStreamsAsDatagrams enables routing of application-level layers in the TCP
    // decoder. If true, we should try to decode layers after TCP in single packets.
    // This is disabled by default because the reassembly package drives the decoding
    // of TCP payload data after reassembly.
    bool DecodeStreamsAsDatagrams;
};

// DecodeFeedback is used by DecodingLayer layers to provide decoding metadata.
struct DecodeFeedback {
    // SetTruncated should be called if during decoding you notice that a packet
    // is shorter than internal layer variables (HeaderLength, or the like) say it
    // should be.  It sets packet.Metadata().Truncated.
    virtual void SetTruncated() = 0;
};

// PacketBuilder is used by layer decoders to store the layers they've decoded,
// and to defer future decoding via NextDecoder.
struct PacketBuilder : public DecodeFeedback {
    // DecodeFeedback
    // AddLayer should be called by a decoder immediately upon successful
    // decoding of a layer.
    virtual void AddLayer(Layer* l) = 0;
    // The following functions set the various specific layers in the final
    // packet.  Note that if many layers call SetX, the first call is kept and all
    // other calls are ignored.
    virtual void SetLinkLayer(LinkLayer) = 0;
    virtual void SetNetworkLayer(NetworkLayer) = 0;
    virtual void SetTransportLayer(TransportLayer) = 0;
    virtual void SetApplicationLayer(ApplicationLayer) = 0;
    virtual void SetErrorLayer(ErrorLayer) = 0;
    // NextDecoder should be called by a decoder when they're done decoding a
    // packet layer but not done with decoding the entire packet.  The next
    // decoder will be called to decode the last AddLayer's LayerPayload.
    // Because of this, NextDecoder must only be called once all other
    // PacketBuilder calls have been made.  Set*Layer and AddLayer calls after
    // NextDecoder calls will behave incorrectly.
    virtual error NextDecoder(Decoder* next) = 0;
    // DumpPacketData is used solely for decoding.  If you come across an error
    // you need to diagnose while processing a packet, call this and your packet's
    // data will be dumped to stderr so you can create a test.  This should never
    // be called from a production decoder.
    virtual void DumpPacketData() = 0;
    // DecodeOptions returns the decode options
    virtual DecodeOptions* DecodeOptions() = 0;
};

// Decoder is an interface for logic to decode a packet layer.  Users may
// implement a Decoder to handle their own strange packet types, or may use one
// of the many decoders available in the 'layers' subpackage to decode things
// for them.
struct Decoder {
    // Decode decodes the bytes of a packet, sending decoded values and other
    // information to PacketBuilder, and returning an error if unsuccessful.  See
    // the PacketBuilder documentation for more details.
    virtual error Decode(bytez<>, PacketBuilder*) = 0;
};

// DecodeFunc wraps a function to make it a Decoder.
// type DecodeFunc func([] byte, PacketBuilder) error

}  // namespace gopacket
}  // namespace nx
