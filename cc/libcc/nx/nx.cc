//
// weproxy@foxmail.com 2022/10/03
//

#include "nx.h"

#include <atomic>

#include "gx/gx.h"

namespace nx {

// NewID ...
uint64 NewID() {
    static std::atomic<uint64> id{0};
    id++;
    return id;
}

}  // namespace nx
