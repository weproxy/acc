//
// weproxy@foxmail.com 2022/10/03
//

#include "sni.h"

namespace nx {
namespace sni {

// GetServerName ...
R<string, error> GetServerName(const void* buf, size_t len) {
    if (len < 16) {
        return {"", io::ErrShortBuffer};
    }

    byte* p = (byte*)buf;

    bool isTLS = p[0] == 0x16;
    if (isTLS) {
    } else {
    }

    return {"", io::ErrShortBuffer};
}

}  // namespace sni
}  // namespace nx
