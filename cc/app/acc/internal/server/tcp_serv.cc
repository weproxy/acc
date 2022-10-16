//
// weproxy@foxmail.com 2022/10/03
//

#include "server.h"

#include "gx/io/io.h"

namespace app {
namespace server {

// tcpServer ...
struct tcpServer : public io::xx::closer_t {
    virtual void Close() override {}
};

}  // namespace server
}  // namespace app
