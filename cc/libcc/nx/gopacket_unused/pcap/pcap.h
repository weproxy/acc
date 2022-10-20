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

////////////////////////////////////////////////////////////////////////////////

// errorBufferSize ...
constexpr int errorBufferSize = PCAP_ERRBUF_SIZE;

// Error code ...
constexpr int pcapErrorNotActivated = PCAP_ERROR_NOT_ACTIVATED;
constexpr int pcapErrorActivated = PCAP_ERROR_ACTIVATED;
constexpr int pcapWarningPromisc = PCAP_WARNING_PROMISC_NOTSUP;
constexpr int pcapErrorNoSuchDevice = PCAP_ERROR_NO_SUCH_DEVICE;
constexpr int pcapErrorDenied = PCAP_ERROR_PERM_DENIED;
constexpr int pcapErrorNotUp = PCAP_ERROR_IFACE_NOT_UP;
constexpr int pcapWarning = PCAP_WARNING;
constexpr int pcapError = PCAP_ERROR;
constexpr int pcapDIN = PCAP_D_IN;
constexpr int pcapDOUT = PCAP_D_OUT;
constexpr int pcapDINOUT = PCAP_D_INOUT;
constexpr int pcapNetmaskUnknown = PCAP_NETMASK_UNKNOWN;
constexpr int pcapTstampPrecisionMicro = PCAP_TSTAMP_PRECISION_MICRO;
constexpr int pcapTstampPrecisionNano = PCAP_TSTAMP_PRECISION_NANO;

// pcapPkthdr ...
struct pcapPkthdr {
    struct pcap_pkthdr h;

    int64 getSec() { return int64(h.ts.tv_sec); }
    int64 getUsec() { return int64(h.ts.tv_usec); }
    int getLen() { return int(h.len); }
    int getCaplen() { return int(h.caplen); }
};

// BPFInstruction is a byte encoded structure holding a BPF instruction
struct BPFInstruction {
    uint16 Code{0};
    uint8 Jt{0};
    uint8 Jf{0};
    uint32 K{0};
};

using pcapTPtr = struct pcap*;

int pcapGetTstampPrecision(pcapTPtr cptr);
error pcapSetTstampPrecision(pcapTPtr cptr, int precision);

// pcapBpfProgram ...
struct pcapBpfProgram {
    struct bpf_program p;

    void free() { pcap_freecode((struct bpf_program*)(&p)); }

    slice<BPFInstruction> toBPFInstruction() {
        auto bpfInstruction = make<BPFInstruction>(p.bf_len, p.bf_len);
        for (int i = 0; i < p.bf_len; i++) {
            auto v = p.bf_insns[i];
            bpfInstruction[i].Code = uint16(v.code);
            bpfInstruction[i].Jt = uint8(v.jt);
            bpfInstruction[i].Jf = uint8(v.jf);
            bpfInstruction[i].K = uint32(v.k);
        }
        return bpfInstruction;
    }
};

////////////////////////////////////////////////////////////////////////////////
// ErrNotActive ...
constexpr int ErrNotActive = pcapErrorNotActivated;

constexpr int MaxBpfInstructions = 4096;

constexpr int bpfInstructionBufferSize = 8 * MaxBpfInstructions;

// Direction is used by Handle.SetDirection.
using Direction = uint8;

// Direction values for Handle.SetDirection.
constexpr Direction DirectionIn = Direction(pcapDIN);
constexpr Direction DirectionOut = Direction(pcapDOUT);
constexpr Direction DirectionInOut = Direction(pcapDINOUT);

////////////////////////////////////////////////////////////////////////////////

// InterfaceAddress describes an address associated with an Interface.
// Currently, it's IPv4/6 specific.
struct InterfaceAddress {
    net::IP IP;           //
    net::IPMask Netmask;  // Netmask may be nil if we were unable to retrieve it.
    net::IP Broadaddr;    // Broadcast address for this IP may be nil
    net::IP P2P;          // P2P destination address for this IP may be nil
};

// Stats contains statistics on how many packets were handled by a pcap handle,
// and what was done with those packets.
struct Stats {
    int PacketsReceived{0};
    int PacketsDropped{0};
    int PacketsIfDropped{0};
};

// Datalink describes the datalink
struct Datalink {
    string Name;
    string Description;
};

// bpfFilter keeps C.struct_bpf_program separate from BPF.orig which might be a pointer to go memory.
// This is a workaround for https://github.com/golang/go/issues/32970 which will be fixed in go1.14.
// (type conversion is in pcap_unix.go pcapOfflineFilter)
struct bpfFilter {
    pcapBpfProgram bpf;  // takes a finalizer, not overriden by outsiders
};

// BPF is a compiled filter program, useful for offline packet matching.
struct BPF {
    string orig;
    Ref<bpfFilter> bpf;
    pcapPkthdr hdr;  // allocate on the heap to enable optimizations

    // String returns the original string this BPF filter was compiled from.
    string String() const;

    // Matches returns true if the given packet data matches this filter.
    bool Matches(gopacket::CaptureInfo ci, slice<> data);
};

// Handle provides a connection to a pcap handle, allowing users to read packets
// off the wire (Next), inject packets onto the wire (Inject), and
// perform a number of other functions to affect and understand packet output.
//
// Handles are already pcap_activate'd
struct Handle : public io::xx::closer_t {
    Handle(pcapTPtr p) : cptr(p) {}

    // stop is set to a non-zero value by Handle.Close to signal to
    // getNextBufPtrLocked to stop trying to read packets
    // This must be the first entry to ensure alignment for sync.atomic
    uint64 stop{0};
    // cptr is the handle for the actual pcap C object.
    pcapTPtr cptr{0};
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

    // Close closes the underlying pcap handle.
    error Close();

    // Error returns the current error associated with a pcap handle (pcap_geterr).
    error Error();

    // Stats returns statistics on the underlying pcap handle.
    R<Ref<Stats>, error> Stats();

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
    R<Ref<BPF>, error> NewBPF(const string& expr);

    // NewBPFInstructionFilter sets the given BPFInstructions as new filter program.
    //
    // More details see func SetBPFInstructionFilter
    //
    // BPF filters need to be created from activated handles, because they need to
    // know the underlying link type to correctly compile their offsets.
    R<Ref<BPF>, error> NewBPFInstructionFilter(slice<BPFInstruction> bpfInstructions);

    // SetDirection sets the direction for which packets will be captured.
    error SetDirection(Direction direction);

    // SnapLen returns the snapshot length
    int SnapLen();

    // ReadPacketData returns the next packet read from the pcap handle, along with an error
    // code associated with that packet.  If the packet is read successfully, the
    // returned error is nil.
    R<slice<>, gopacket::CaptureInfo, error> ReadPacketData();

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
    R<slice<>, gopacket::CaptureInfo, error> ZeroCopyReadPacketData();

    // WritePacketData calls pcap_sendpacket, injecting the given data into the pcap handle.
    error WritePacketData(slice<> data);

   public:
    error setNonBlocking();

    R<Ref<struct Stats>, error> pcapStats();

    error pcapGeterr();
    void pcapClose();

    R<pcapBpfProgram, error> pcapCompile(const string& expr, uint32 maskp);

    void waitForPacket();
};

// Interface describes a single network interface on a machine.
struct Interface {
    string Name;
    string Description;
    uint32 Flags{0};
    slice<InterfaceAddress> Addresses;
};

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

R<Ref<Handle>, error> pcapOpenLive(const string& device, int snaplen, int pro, int timeout);
R<Ref<Handle>, error> openOffline(const string& file);

// OpenLive opens a device and returns a *Handle.
// It takes as arguments the name of the device ("eth0"), the maximum size to
// read for each packet (snaplen), whether to put the interface in promiscuous
// mode, and a timeout. Warning: this function supports only microsecond timestamps.
// For nanosecond resolution use an InactiveHandle.
//
// See the package documentation for important details regarding 'timeout'.
R<Ref<Handle>, error> OpenLive(const string& device, int snaplen, bool promisc, time::Duration timeout);

// OpenOffline opens a file and returns its contents as a *Handle. Depending on libpcap support and
// on the timestamp resolution used in the file, nanosecond or microsecond resolution is used
// internally. All returned timestamps are scaled to nanosecond resolution. Resolution() can be used
// to query the actual resolution used.
R<Ref<Handle>, error> OpenOffline(const string& file);

// CompileBPFFilter compiles and returns a BPF filter with given a link type and capture length.
R<slice<BPFInstruction>, error> CompileBPFFilter(layers::LinkType linkType, int captureLength, const string& expr);

// NewBPF allows to create a BPF without requiring an existing handle.
// This allows to match packets obtained from a-non GoPacket capture source
// to be matched.
R<Ref<BPF>, error> NewBPF(layers::LinkType linkType, int captureLength, const string& expr);

}  // namespace pcap
}  // namespace nx
