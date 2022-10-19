//
// weproxy@foxmail.com 2022/10/03
//

#include "c_unix.cc.inl"
#include "pcap.h"

////////////////////////////////////////////////////////////////

namespace nx {
namespace pcap {

// pcapGetTstampPrecision ...
int pcapGetTstampPrecision(pcapTPtr cptr) { return pcap_get_tstamp_precision(cptr); }

// pcapSetTstampPrecision ...
error pcapSetTstampPrecision(pcapTPtr cptr, int precision) {
    int ret = pcap_set_tstamp_precision(cptr, precision);
    if (ret < 0) {
        return errors::New(pcap_geterr(cptr));
    }
    return nil;
}

// pcapOpenLive ..
R<Ref<Handle>, error> pcapOpenLive(const string& device, int snaplen, int pro, int timeout) {
    char* msg = (char*)calloc(errorBufferSize, 1);
    DEFER(free(msg));

    pcap_t* cptr = pcap_open_live(device.c_str(), snaplen, pro, timeout, msg);
    if (!cptr) {
        return {nil, errors::New(msg)};
    }
    return {NewRef<Handle>(cptr), nil};
}

// openOffline ...
R<Ref<Handle>, error> openOffline(const string& file) {
    char* msg = (char*)calloc(errorBufferSize, 1);
    DEFER(free(msg));

    pcap_t* cptr = pcap_open_offline_with_tstamp_precision(file.c_str(), PCAP_TSTAMP_PRECISION_NANO, msg);
    if (!cptr) {
        return {nil, errors::New(msg)};
    }
    return {NewRef<Handle>(cptr), nil};
}

}  // namespace pcap
}  // namespace nx
