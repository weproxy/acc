//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/fmt/fmt.h"
#include "net.h"
#include "xx.h"

namespace gx {
namespace net {

// error ...
static const error errNotFound = errors::New("not found");
static const error errFuncNotImpl = errors::New("func not impl");

// String ...
string HardwareAddr::String(bool upper) const {
    return fmt::Sprintf(upper ? "%02X:%02X:%02X:%02X:%02X:%02X" : "%02x:%02x:%02x:%02x:%02x:%02x", B[0], B[1], B[2],
                        B[3], B[4], B[5]);
}

namespace xx {
// Addrs ...
R<Vec<Addr>, error> interface_t::Addrs() { return {{}, errFuncNotImpl}; }
}  // namespace xx

// Interfaces ...
R<Vec<Interface>, error> Interfaces() { return {{}, errFuncNotImpl}; }

// InterfaceAddrs ...
R<Vec<Addr>, error> InterfaceAddrs() { return {{}, errFuncNotImpl}; }

// InterfaceByIndex ...
R<Interface, error> InterfaceByIndex(int index) { return {nil, errFuncNotImpl}; }

// InterfaceByName ...
R<Interface, error> InterfaceByName(const string& name) { return {nil, errFuncNotImpl}; }

}  // namespace net
}  // namespace gx
