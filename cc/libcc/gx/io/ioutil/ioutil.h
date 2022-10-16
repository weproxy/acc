//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../io.h"

namespace gx {
namespace ioutil {

// ReadAll ...
template <typename IReader, typename std::enable_if<io::xx::is_reader<IReader>::value, int>::type = 0>
R<slice<byte>, error> ReadAll(IReader r) {
    return {{}, nil};
}

// ReadFile ...
inline R<slice<byte>, error> ReadFile(const string& filename) {
    return {{}, nil};
}

// WriteFile ...
inline error WriteFile(const string& filename, const void* data, size_t len) {
    return nil;
}

}  // namespace ioutil
}  // namespace gx
