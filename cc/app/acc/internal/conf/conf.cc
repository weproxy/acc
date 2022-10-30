//
// weproxy@foxmail.com 2022/10/03
//

#include "conf.h"

#include "conf.json.h"
#include "gx/encoding/json/json.h"

namespace internal {
namespace conf {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Auth, s5, ss)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Server, auth, dns, main, geo)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Rule, host, serv)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Conf, server, dns, rules)

// _default ...
static Ref<Conf> _default = []() -> Ref<Conf> {
    AUTO_R(c, _, Parse("default", _DEFAULT_CONF));
    return c;
}();

// Default ...
Ref<Conf> Default() { return _default; }

// String ...
string Conf::String() const {
    AUTO_R(str, err, gx::json::Marshal(*this));
    return err ? "{}" : str;
}

// Parse ...
error Conf::Parse(const string& js) {
    // parse
    return gx::json::Unmarshal(js, this);
}

////////////////////////////////////////////////////////////////////////////////

// _confs ...
static auto _confs = makemap<string, Ref<Conf>>();

// MustGet ...
Ref<Conf> MustGet(const string& key) {
    {
        AUTO_R(c, ok, _confs(key));
        if (ok) {
            return c;
        }
    }
    {
        AUTO_R(c, ok, _confs("default"));
        if (ok) {
            return c;
        }
    }

    AUTO_R(c, err, Parse("default", _DEFAULT_CONF));

    if (err == nil) {
        return c;
    }

    return NewRef<Conf>();
}

// Parse ...
R<Ref<Conf>, error> Parse(const string& key, const string& js) {
    auto c = NewRef<Conf>();
    auto err = c->Parse(js);
    if (err) {
        return {nil, err};
    }
    return {c, nil};
}

}  // namespace conf
}  // namespace internal
