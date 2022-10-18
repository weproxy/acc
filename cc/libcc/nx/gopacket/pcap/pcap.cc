//
// weproxy@foxmail.com 2022/10/03
//

#include "pcap.h"

namespace nx {
namespace pcap {

// OpenLive opens a device and returns a *Handle.
// It takes as arguments the name of the device ("eth0"), the maximum size to
// read for each packet (snaplen), whether to put the interface in promiscuous
// mode, and a timeout. Warning: this function supports only microsecond timestamps.
// For nanosecond resolution use an InactiveHandle.
//
// See the package documentation for important details regarding 'timeout'.
R<Handle, error> OpenLive(const string& device, int32 snaplen, bool promisc, time::Duration timeout) {
    return {nil, gx_TodoErr()};
}

// OpenOffline opens a file and returns its contents as a *Handle. Depending on libpcap support and
// on the timestamp resolution used in the file, nanosecond or microsecond resolution is used
// internally. All returned timestamps are scaled to nanosecond resolution. Resolution() can be used
// to query the actual resolution used.
R<Handle, error> OpenOffline(const string& file) { return {nil, gx_TodoErr()}; }

// CompileBPFFilter compiles and returns a BPF filter with given a link type and capture length.
R<slice<BPFInstruction>, error> CompileBPFFilter(layers::LinkType linkType, int captureLength, const string& expr) {
    return {{}, gx_TodoErr()};
}

// NewBPF allows to create a BPF without requiring an existing handle.
// This allows to match packets obtained from a-non GoPacket capture source
// to be matched.
R<BPF, error> NewBPF(layers::LinkType linkType, int captureLength, const string& expr) { return {nil, gx_TodoErr()}; }

// FindAllDevs attempts to enumerate all interfaces on the current machine.
R<slice<Interface>, error> FindAllDevs();


}  // namespace pcap
}  // namespace nx
