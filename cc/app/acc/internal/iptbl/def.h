//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"

namespace internal {
namespace iptbl {

// ProxyMode ...
enum class ProxyMode : int {
    None = 0,
    TUN,
    DNAT,
    TPROXY,
    PCAP,
    WFP,
    TUNFD,
    RAW,
};

// ToString ...
inline const char* ToString(const ProxyMode e) {
#define CASE_RETURN_PROXYMODE(x) \
    case ProxyMode::x:           \
        return "ProxyMode" #x
#define CASE_RETURN_PROXYDEFT(x) \
    default:                     \
        return "ProxyMode" #x

    switch (e) {
        CASE_RETURN_PROXYMODE(TUN);
        CASE_RETURN_PROXYMODE(DNAT);
        CASE_RETURN_PROXYMODE(TPROXY);
        CASE_RETURN_PROXYMODE(PCAP);
        CASE_RETURN_PROXYMODE(WFP);
        CASE_RETURN_PROXYMODE(TUNFD);
        CASE_RETURN_PROXYMODE(RAW);
        CASE_RETURN_PROXYDEFT(None);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// FilterType ...
enum class FilterType : int {
    None = 0,
    Whitelist,
    Blacklist,
    Both,
};

// ToString ...
inline const char* ToString(const FilterType e) {
#define CASE_RETURN_FILTERTYPE(x) \
    case FilterType::x:           \
        return "FilterType" #x
#define CASE_RETURN_FILTERDEFT(x) \
    default:                      \
        return "FilterType" #x

    switch (e) {
        CASE_RETURN_FILTERTYPE(Both);
        CASE_RETURN_FILTERTYPE(Whitelist);
        CASE_RETURN_FILTERTYPE(Blacklist);
        CASE_RETURN_FILTERDEFT(None);
    }
}

}  // namespace iptbl
}  // namespace internal

////////////////////////////////////////////////////////////////////////////////
namespace std {
// override ostream <<
inline ostream& operator<<(ostream& o, const internal::iptbl::ProxyMode v) { return o << internal::iptbl::ToString(v); }
inline ostream& operator<<(ostream& o, const internal::iptbl::FilterType v) {
    return o << internal::iptbl::ToString(v);
}
}  // namespace std
