//
// weproxy@foxmail.com 2022/10/03
//

#include "net.h"

namespace gx {
namespace net {

// Error
const error ErrClosed = errors::New("use of closed network connection");

namespace xx {

// ToString ...
string ToString(Flags f) {
    static const char* flagNames[] = {
        "up", "broadcast", "loopback", "pointtopoint", "multicast",
    };

    string s;
    for (int i = 0; i < sizeof(flagNames) / sizeof(flagNames[0]); i++) {
        if ((f & (1 << i)) != 0) {
            if (!s.empty()) {
                s += "|";
            }
            s += flagNames[i];
        }
    }
    if (s.empty()) {
        s = "0";
    }
    return s;
}

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
