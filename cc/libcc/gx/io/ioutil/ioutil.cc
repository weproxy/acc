//
// weproxy@foxmail.com 2022/10/03
//

#include "ioutil.h"

namespace gx {
namespace ioutil {

// ReadFile ...
R<slice<byte>, error> ReadFile(const string& filename) {
    // TOOD..
    return {{}, gx_TodoErr()};
}

// WriteFile ...
error WriteFile(const string& filename, const void* data, size_t size) {
    // TOOD..
    return gx_TodoErr();
}

}  // namespace ioutil
}  // namespace gx