//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../io.h"

namespace gx {
namespace ioutil {

// ReadAll ...
template <typename Reader, typename std::enable_if<io::xx::has_read<Reader>::value, int>::type = 0>
R<slice<byte>, error> ReadAll(Reader r) {
    return {{}, ERR_TODO};
}

// ReadFile ...
inline R<slice<byte>, error> ReadFile(const string& filename) {
    // TOOD..
    return {{}, ERR_TODO};
}

// WriteFile ...
inline error WriteFile(const string& filename, const void* data, size_t len) {
    // TOOD..
    return ERR_TODO;
}

}  // namespace ioutil
}  // namespace gx
