//
// weproxy@foxmail.com 2022/10/03
//

#include "os.h"

#include "gx/fmt/fmt.h"

namespace gx {
namespace os {

// ReadFile ...
R<bytez<>, error> ReadFile(const string& filename) {
    // TOOD..
    return {{}, gx_TodoErr()};
}

// WriteFile ...
error WriteFile(const string& filename, const void* data, size_t size) {
    // TOOD..
    return gx_TodoErr();
}

}  // namespace os
}  // namespace gx
