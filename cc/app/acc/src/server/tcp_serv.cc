//
// weproxy@foxmail.com 2022/10/03
//

#include "def.h"

namespace app {
namespace server {

// tcpServer ...
struct tcpServer : public io::ICloser {
    virtual void Close() override {}
};

}  // namespace server
}  // namespace app
