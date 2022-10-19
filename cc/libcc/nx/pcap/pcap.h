//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <pcap/pcap.h>

#include "gx/io/io.h"
#include "gx/net/net.h"
#include "gx/sync/sync.h"
#include "gx/time/time.h"

namespace nx {
namespace pcap {
using namespace gx;

// InterfaceAddress describes an address associated with an Interface.
// Currently, it's IPv4/6 specific.
struct InterfaceAddress {
    net::IP IP;           //
    net::IPMask Netmask;  // Netmask may be nil if we were unable to retrieve it.
    net::IP Broadaddr;    // Broadcast address for this IP may be nil
    net::IP P2P;          // P2P destination address for this IP may be nil
};

// Interface describes a single network interface on a machine.
struct Interface {
    string Name;
    string Description;
    uint32 Flags{0};
    slice<InterfaceAddress> Addresses;
};

// BPF is a compiled filter program, useful for offline packet matching.
struct BPF {
};

// Handle provides a connection to a pcap handle, allowing users to read packets
// off the wire (Next), inject packets onto the wire (Inject), and
// perform a number of other functions to affect and understand packet output.
//
// Handles are already pcap_activate'd
struct Handle {

};

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

// FindAllDevs attempts to enumerate all interfaces on the current machine.
R<slice<Interface>, error> FindAllDevs();

}  // namespace pcap
}  // namespace nx
