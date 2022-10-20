//
// weproxy@foxmail.com 2022/10/03
//

#include "nx.h"

#include <atomic>

#include "gx/fmt/fmt.h"

namespace nx {

// NewID ...
uint64 NewID() {
    static std::atomic<uint64> id{0};
    id++;
    return id;
}

// BindInterface ...
error BindInterface(int fd, int ifaceInex) { return gx_TodoErr(); }

}  // namespace nx
