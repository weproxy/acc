//
// weproxy@foxmail.com 2022/10/03
//

#include "conf.h"

#include "gx/encoding/json/json.h"

namespace internal {
namespace conf {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Auth, s5, ss)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Server, auth, dns, main, geo)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Rule, host, serv)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Conf, server, dns, rules)

// _default ...
static Ref<Conf> _default(NewRef<Conf>());

// Default ...
Ref<Conf> Default() { return _default; }

// String ...
string Conf::String() const {
    AUTO_R(str, err, gx::json::Marshal(*this));
    return err ? "{}" : str;
}

// ParseJSON ...
error Conf::ParseJSON(const string& jsonContent) {
    //
    return gx::json::Unmarshal(jsonContent, this);
}

// NewFromJSON ...
R<Ref<Conf>, error> NewFromJSON(const string& jsonContent) {
    auto c = NewRef<Conf>();
    auto err = c->ParseJSON(jsonContent);
    if (err) {
        return {nil, err};
    }
    return {c, nil};
}

}  // namespace conf
}  // namespace internal
