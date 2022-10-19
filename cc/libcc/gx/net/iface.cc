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

// String ...
string HardwareAddr::String(bool upper) const {
    return fmt::Sprintf(upper ? "%02X:%02X:%02X:%02X:%02X:%02X" : "%02x:%02x:%02x:%02x:%02x:%02x", B[0], B[1], B[2],
                        B[3], B[4], B[5]);
}

// Addrs ...
R<slice<Addr>, error> Interface::Addrs() const { return {{}, gx_TodoErr()}; }

// Interfaces ...
R<slice<Ref<Interface>>, error> Interfaces() { return {{}, gx_TodoErr()}; }

// InterfaceAddrs ...
R<slice<Addr>, error> InterfaceAddrs() { return {{}, gx_TodoErr()}; }

// InterfaceByIndex ...
R<Ref<Interface>, error> InterfaceByIndex(int index) { return {nil, gx_TodoErr()}; }

// InterfaceByName ...
R<Ref<Interface>, error> InterfaceByName(const string& name) { return {nil, gx_TodoErr()}; }

}  // namespace net
}  // namespace gx
