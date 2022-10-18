//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <pcap/pcap.h>

#include "gx/io/io.h"
#include "gx/net/net.h"
#include "gx/sync/sync.h"
#include "gx/time/time.h"
#include "nx/gopacket/gopacket.h"
#include "nx/gopacket/layers/layers.h"

namespace nx {
namespace pcap {
using namespace gx;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// errorBufferSize ...
const int errorBufferSize = PCAP_ERRBUF_SIZE;

// Error code ...
const int pcapErrorNotActivated = PCAP_ERROR_NOT_ACTIVATED;
const int pcapErrorActivated = PCAP_ERROR_ACTIVATED;
const int pcapWarningPromisc = PCAP_WARNING_PROMISC_NOTSUP;
const int pcapErrorNoSuchDevice = PCAP_ERROR_NO_SUCH_DEVICE;
const int pcapErrorDenied = PCAP_ERROR_PERM_DENIED;
const int pcapErrorNotUp = PCAP_ERROR_IFACE_NOT_UP;
const int pcapWarning = PCAP_WARNING;
const int pcapError = PCAP_ERROR;
const int pcapDIN = PCAP_D_IN;
const int pcapDOUT = PCAP_D_OUT;
const int pcapDINOUT = PCAP_D_INOUT;
const int pcapNetmaskUnknown = PCAP_NETMASK_UNKNOWN;
const int pcapTstampPrecisionMicro = PCAP_TSTAMP_PRECISION_MICRO;
const int pcapTstampPrecisionNano = PCAP_TSTAMP_PRECISION_NANO;

using pcapPkthdr = struct pcap_pkthdr;
using pcapTPtr = struct pcap;
using pcapBpfProgram = struct bpf_program;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ErrNotActive ...
const int ErrNotActive = pcapErrorNotActivated;

const int MaxBpfInstructions = 4096;

const int bpfInstructionBufferSize = 8 * MaxBpfInstructions;

// Direction is used by Handle.SetDirection.
using Direction = uint8;

// Direction values for Handle.SetDirection.
const Direction DirectionIn = Direction(pcapDIN);
const Direction DirectionOut = Direction(pcapDOUT);
const Direction DirectionInOut = Direction(pcapDINOUT);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// InterfaceAddress describes an address associated with an Interface.
// Currently, it's IPv4/6 specific.
struct interfaceAddress_t {
    net::IP IP;           //
    net::IPMask Netmask;  // Netmask may be nil if we were unable to retrieve it.
    net::IP Broadaddr;    // Broadcast address for this IP may be nil
    net::IP P2P;          // P2P destination address for this IP may be nil
};

using InterfaceAddress = Ref<interfaceAddress_t>;

// Stats contains statistics on how many packets were handled by a pcap handle,
// and what was done with those packets.
struct stats_t {
    int PacketsReceived{0};
    int PacketsDropped{0};
    int PacketsIfDropped{0};
};

// Stats ...
using Stats = Ref<stats_t>;

// Datalink describes the datalink
struct datalink_t {
    string Name;
    string Description;
};

// Datalink ...
using Datalink = Ref<datalink_t>;

// bpfFilter keeps C.struct_bpf_program separate from BPF.orig which might be a pointer to go memory.
// This is a workaround for https://github.com/golang/go/issues/32970 which will be fixed in go1.14.
// (type conversion is in pcap_unix.go pcapOfflineFilter)
struct bpfFilter {
    pcapBpfProgram bpf;  // takes a finalizer, not overriden by outsiders
};

// BPF is a compiled filter program, useful for offline packet matching.
struct bpf_t {
    string orig;
    bpfFilter* bpf{0};
    pcapPkthdr hdr;  // allocate on the heap to enable optimizations

    // String returns the original string this BPF filter was compiled from.
    string String() const;

    // Matches returns true if the given packet data matches this filter.
    bool Matches(gopacket::CaptureInfo ci, slice<byte> data);
};

// BPF ...
using BPF = Ref<bpf_t>;

// BPFInstruction is a byte encoded structure holding a BPF instruction
struct bpfInstruction_t {
    uint16 Code{0};
    uint8 Jt{0};
    uint8 Jf{0};
    uint32 K{0};
};

// BPFInstruction ...
using BPFInstruction = Ref<bpfInstruction_t>;

// Handle provides a connection to a pcap handle, allowing users to read packets
// off the wire (Next), inject packets onto the wire (Inject), and
// perform a number of other functions to affect and understand packet output.
//
// Handles are already pcap_activate'd
struct handler_t : public io::xx::closer_t {
    // stop is set to a non-zero value by Handle.Close to signal to
    // getNextBufPtrLocked to stop trying to read packets
    // This must be the first entry to ensure alignment for sync.atomic
    uint64 stop{0};
    // cptr is the handle for the actual pcap C object.
    pcapTPtr* cptr{0};
    time::Duration timeout;
    string device;
    int deviceIndex{-1};
    sync::Mutex mu;
    sync::Mutex closeMu;
    int64 nanoSecsFactor{0};

    // Since pointers to these objects are passed into a C function, if
    // they're declared locally then the Go compiler thinks they may have
    // escaped into C-land, so it allocates them on the heap.  This causes a
    // huge memory hit, so to handle that we store them here instead.
    pcapPkthdr* pkthdr{0};
    uint8* bufptr{0};

    // ReadPacketData returns the next packet read from the pcap handle, along with an error
    // code associated with that packet.  If the packet is read successfully, the
    // returned error is nil.
    R<slice<byte>, gopacket::CaptureInfo, error> ReadPacketData();

    // ZeroCopyReadPacketData reads the next packet off the wire, and returns its data.
    // The slice returned by ZeroCopyReadPacketData points to bytes owned by the
    // the Handle.  Each call to ZeroCopyReadPacketData invalidates any data previously
    // returned by ZeroCopyReadPacketData.  Care must be taken not to keep pointers
    // to old bytes when using ZeroCopyReadPacketData... if you need to keep data past
    // the next time you call ZeroCopyReadPacketData, use ReadPacketData, which copies
    // the bytes into a new buffer for you.
    //  data1, _, _ := handle.ZeroCopyReadPacketData()
    //  // do everything you want with data1 here, copying bytes out of it if you'd like to keep them around.
    //  data2, _, _ := handle.ZeroCopyReadPacketData()  // invalidates bytes in data1
    R<slice<byte>, gopacket::CaptureInfo, error> ReadPZeroCopyReadPacketDataacketData();

    // Close closes the underlying pcap handle.
    error Close();

    // Error returns the current error associated with a pcap handle (pcap_geterr).
    error Error();

    // Stats returns statistics on the underlying pcap handle.
    R<Stats, error> Stats();

    // ListDataLinks obtains a list of all possible data link types supported for an interface.
    R<slice<Datalink>, error> ListDataLinks();

    // CompileBPFFilter compiles and returns a BPF filter for the pcap handle.
    R<slice<BPFInstruction>, error> CompileBPFFilter(const string& expr);

    // SetBPFFilter compiles and sets a BPF filter for the pcap handle.
    error SetBPFFilter(const string& expr);

    // SetBPFInstructionFilter may be used to apply a filter in BPF asm byte code format.
    error SetBPFInstructionFilter(slice<BPFInstruction> bpfInstructions);

    // NewBPF compiles the given string into a new filter program.
    //
    // BPF filters need to be created from activated handles, because they need to
    // know the underlying link type to correctly compile their offsets.
    R<BPF, error> NewBPF(const string& expr);

    // NewBPFInstructionFilter sets the given BPFInstructions as new filter program.
    //
    // More details see func SetBPFInstructionFilter
    //
    // BPF filters need to be created from activated handles, because they need to
    // know the underlying link type to correctly compile their offsets.
    R<BPF, error> NewBPFInstructionFilter(slice<BPFInstruction> bpfInstructions);

    // WritePacketData calls pcap_sendpacket, injecting the given data into the pcap handle.
    error WritePacketData(slice<byte> data);

    // SetDirection sets the direction for which packets will be captured.
    error SetDirection(Direction direction);

    // SnapLen returns the snapshot length
    int SnapLen();
};

// Handle ...
using Handle = Ref<handler_t>;

// Interface describes a single network interface on a machine.
struct interface_t {
    string Name;
    string Description;
    uint32 Flags{0};
    slice<InterfaceAddress> Addresses;
};

// Interface ...
using Interface = Ref<interface_t>;

// BlockForever causes it to block forever waiting for packets, when passed
// into SetTimeout or OpenLive, while still returning incoming packets to userland relatively
// quickly.
const int64 BlockForever = -time::Millisecond * 10;

// timeoutMillis ...
inline int timeoutMillis(time::Duration timeout) {
    // Flip sign if necessary.  See package docs on timeout for reasoning behind this.
    if (timeout < 0) {
        timeout = -1 * timeout;
    }
    // Round up
    if (timeout != 0 && timeout < time::Millisecond) {
        timeout = time::Millisecond;
    }
    return int(timeout / time::Millisecond);
}

// OpenLive opens a device and returns a *Handle.
// It takes as arguments the name of the device ("eth0"), the maximum size to
// read for each packet (snaplen), whether to put the interface in promiscuous
// mode, and a timeout. Warning: this function supports only microsecond timestamps.
// For nanosecond resolution use an InactiveHandle.
//
// See the package documentation for important details regarding 'timeout'.
R<Handle, error> OpenLive(const string& device, int32 snaplen, bool promisc, time::Duration timeout);

// OpenOffline opens a file and returns its contents as a *Handle. Depending on libpcap support and
// on the timestamp resolution used in the file, nanosecond or microsecond resolution is used
// internally. All returned timestamps are scaled to nanosecond resolution. Resolution() can be used
// to query the actual resolution used.
R<Handle, error> OpenOffline(const string& file);

// CompileBPFFilter compiles and returns a BPF filter with given a link type and capture length.
R<slice<BPFInstruction>, error> CompileBPFFilter(layers::LinkType linkType, int captureLength, const string& expr);

// NewBPF allows to create a BPF without requiring an existing handle.
// This allows to match packets obtained from a-non GoPacket capture source
// to be matched.
R<BPF, error> NewBPF(layers::LinkType linkType, int captureLength, const string& expr);

}  // namespace pcap
}  // namespace nx
