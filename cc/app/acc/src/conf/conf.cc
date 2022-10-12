//
// weproxy@foxmail.com 2022/10/03
//

#include "conf.h"

#include "gx/encoding/json/json.h"

namespace app {
namespace conf {
namespace xx {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(auth_t, s5, ss)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(server_t, auth, tcp, udp, geo)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(rule_t, host, serv)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(conf_t, server, rules)

// _default ...
static Conf _default(new conf_t());

// Default ...
Conf Default() { return _default; }

// String ...
string conf_t::String() const {
    AUTO_R(str, err, gx::json::Marshal(*this));
    return err ? "{}" : str;
}

// ParseJSON ...
error conf_t::ParseJSON(const string& jsonContent) {
    return gx::json::Unmarshal(jsonContent, this);
}

}  // namespace xx

// NewFromJSON ...
R<Conf, error> NewFromJSON(const string& jsonContent) {
    Conf c(new xx::conf_t);
    auto err = c->ParseJSON(jsonContent);
    if (err) {
        return {nil, err};
    }
    return {c, nil};
}

}  // namespace conf
}  // namespace app
