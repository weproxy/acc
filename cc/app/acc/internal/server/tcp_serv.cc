//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/io/io.h"
#include "server.h"

namespace app {
namespace server {

// tcpServer ...
struct tcpServer : public io::xx::closer_t {
    virtual error Close() override { return nil; }
};

}  // namespace server
}  // namespace app
