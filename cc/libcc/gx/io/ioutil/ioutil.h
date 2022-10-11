//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../io.h"

namespace gx {
namespace ioutil {

// ReadAll ...
template <typename Reader, typename std::enable_if<io::xx::is_reader<Reader>::value, int>::type = 0>
R<bytesli, error> ReadAll(Reader r) {
    return {{}, nil};
}

// ReadFile ...
inline R<bytesli, error> ReadFile(const string& filename) {
    return {{}, nil};
}

// WriteFile ...
inline error WriteFile(const string& filename, const void* data, size_t len) {
    return nil;
}

}  // namespace ioutil
}  // namespace gx
