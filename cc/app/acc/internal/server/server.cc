//
// weproxy@foxmail.com 2022/10/03
//

#include "server.h"

#include "gx/fmt/fmt.h"

namespace app {
namespace server {

// Start ...
R<io::Closer, error> Start() {
    error err;

    if (1) {
        err = fmt::Errorf("failed on port: %d", 1122);
    }

    return {nil, err};
}

}  // namespace server
}  // namespace app
