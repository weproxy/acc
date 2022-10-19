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
R<Ref<Handle>, error> OpenLive(const string& device, int snaplen, bool promisc, time::Duration timeout) {
    int pro = promisc ? 1 : 0;

    AUTO_R(p, err, pcapOpenLive(device, snaplen, pro, timeoutMillis(timeout)));
    if (err != nil) {
        return {nil, err};
    }
    p->timeout = timeout;
    p->device = device;

    AUTO_R(ifc, er2, net::InterfaceByName(device));
    if (er2 != nil) {
        // The device wasn't found in the OS, but could be "any"
        // Set index to 0
        p->deviceIndex = 0;
    } else {
        p->deviceIndex = ifc->Index;
    }

    p->nanoSecsFactor = 1000;

    // Only set the PCAP handle into non-blocking mode if we have a timeout
    // greater than zero. If the user wants to block forever, we'll let libpcap
    // handle that.
    if (p->timeout > 0) {
        auto err = p->setNonBlocking();
        if (err != nil) {
            p->pcapClose();
            return {nil, err};
        }
    }

    return {p, nil};
}

// OpenOffline opens a file and returns its contents as a *Handle. Depending on libpcap support and
// on the timestamp resolution used in the file, nanosecond or microsecond resolution is used
// internally. All returned timestamps are scaled to nanosecond resolution. Resolution() can be used
// to query the actual resolution used.
R<Ref<Handle>, error> OpenOffline(const string& file) {
    AUTO_R(handle, err, openOffline(file));
    if (err != nil) {
        return {nil, err};
    }
    if (pcapGetTstampPrecision(handle->cptr) == pcapTstampPrecisionNano) {
        handle->nanoSecsFactor = 1;
    } else {
        handle->nanoSecsFactor = 1000;
    }
    return {handle, nil};
}

// CompileBPFFilter compiles and returns a BPF filter with given a link type and capture length.
R<slice<BPFInstruction>, error> CompileBPFFilter(layers::LinkType linkType, int captureLength, const string& expr) {
    return {{}, gx_TodoErr()};
}

// NewBPF allows to create a BPF without requiring an existing handle.
// This allows to match packets obtained from a-non GoPacket capture source
// to be matched.
R<Ref<BPF>, error> NewBPF(layers::LinkType linkType, int captureLength, const string& expr) {
    return {nil, gx_TodoErr()};
}

// FindAllDevs attempts to enumerate all interfaces on the current machine.
R<slice<Interface>, error> FindAllDevs();

}  // namespace pcap
}  // namespace nx
