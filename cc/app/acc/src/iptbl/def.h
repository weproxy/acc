//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"

namespace app {
namespace iptbl {

// ProxyMode ...
enum ProxyMode {
    ProxyModeNone = 0,
    ProxyModeTUN,
    ProxyModeDNAT,
    ProxyModeTPROXY,
    ProxyModePCAP,
    ProxyModeWFP,
    ProxyModeTUNFD,
    ProxyModeRAW,
};

// String ...
inline const char* String(ProxyMode m) {
#define CASE_PROXYMODE(x) \
    case ProxyMode##x:    \
        return #x
#define CASE_PROXYDEFT(x) \
    default:              \
        return #x

    switch (m) {
        CASE_PROXYMODE(TUN);
        CASE_PROXYMODE(DNAT);
        CASE_PROXYMODE(TPROXY);
        CASE_PROXYMODE(PCAP);
        CASE_PROXYMODE(WFP);
        CASE_PROXYMODE(TUNFD);
        CASE_PROXYMODE(RAW);
        CASE_PROXYDEFT(None);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// FilterType ...
enum FilterType {
    FilterTypeNone = 0,
    FilterTypeWhitelist,
    FilterTypeBlacklist,
    FilterTypeBoth,
};

// String ...
inline const char* String(FilterType m) {
#define CASE_FILTERTYPE(x) \
    case FilterType##x:    \
        return #x
#define CASE_FILTERDEFT(x) \
    default:               \
        return #x

    switch (m) {
        CASE_FILTERTYPE(Both);
        CASE_FILTERTYPE(Whitelist);
        CASE_FILTERTYPE(Blacklist);
        CASE_FILTERDEFT(None);
    }
}

}  // namespace iptbl
}  // namespace app
