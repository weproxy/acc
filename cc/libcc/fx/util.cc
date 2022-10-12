//
// weproxy@foxmail.com 2022/10/03
//

#include "util.h"

#define KB(n) (1024 * int64_t(n))
#define MB(n) (KB(1024) * (n))
#define GB(n) (MB(1024) * (n))
#define TB(n) (GB(1024) * (n))
#define PB(n) (TB(1024) * (n))

namespace fx {

// FormatBytes ...
std::string FormatBytes(int64 n) {
    char buf[64];
    int r;

    if (n >= PB(1)) {
        r = ::snprintf(buf, sizeof(buf), "%.2fPB", float(n) / float(PB(1)));
    } else if (n >= TB(1)) {
        r = ::snprintf(buf, sizeof(buf), "%.2fTB", float(n) / float(TB(1)));
    } else if (n >= GB(1)) {
        r = ::snprintf(buf, sizeof(buf), "%.2fGB", float(n) / float(GB(1)));
    } else if (n >= MB(1)) {
        r = ::snprintf(buf, sizeof(buf), "%.2fMB", float(n) / float(MB(1)));
    } else if (n >= KB(1)) {
        r = ::snprintf(buf, sizeof(buf), "%.2fKB", float(n) / float(KB(1)));
    } else {
        r = ::snprintf(buf, sizeof(buf), "%lldB", n);
    }

    return std::string(buf, r);
}

}  // namespace fx
