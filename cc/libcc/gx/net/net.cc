//
// weproxy@foxmail.com 2022/10/03
//

#include "net.h"

namespace gx {
namespace net {

// Error
const error ErrClosed = errors::New("use of closed network connection");

namespace xx {
// String ...
string addr_t::String() const {
    if (this->IP) {
        return fmt::Sprintf("%s:%d", this->IP.String().c_str(), this->Port);
    }
    return fmt::Sprintf(":%d", this->Port);
}
}  // namespace xx

// SplitHostPort ...
R<string, int, error> SplitHostPort(const string& addr) {
    const char* s = addr.c_str();
    const char* i = ::strrchr(s, ':');
    if (!i) {
        return {"", 0, errors::New("missing port in address")};
    }
    int port = ::atoi(i + 1);
    string host(s, i - s);
    return {host, port, nil};
}

}  // namespace net
}  // namespace gx
