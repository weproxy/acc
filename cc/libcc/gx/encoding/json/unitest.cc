//
// weproxy@foxmail.com 2022/10/03
//

#include "json.h"

namespace gx {
namespace unitest {

// auth_t ...
struct auth_t {
    string s5;
    string ss;
};

// server_t ...
struct server_t {
    auth_t auth;
    strvec tcp;
    strvec udp;
    Map<string, strvec> geo;
};

// rule_t ...
struct rule_t {
    strvec host;
    strvec serv;
};

// conf_t ...
struct conf_t {
    server_t server;
    Vec<rule_t> rules;

    // String ...
    string String() const;
    operator string() const { return String(); }

    // ParseJSON ...
    error ParseJSON(const string& jsonContent);

   private:
    // Fix ...
    error Fix();

   private:
    Vec<rule_t> dnsRules;
    Vec<rule_t> netRules;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(auth_t, s5, ss)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(server_t, auth, tcp, udp, geo)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(rule_t, host, serv)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(conf_t, server, rules)

void test_json() {
    conf_t c;

    c.server.auth.s5 = "s5user:s5pass";
    c.server.auth.ss = "ssuser:sspass";

    c.server.tcp.push_back("s5://1.2.3.4");
    c.server.geo["cn"].push_back("s5://1.2.3.4");
    c.server.geo["us"].push_back("s5://1.2.3.4");

    rule_t r;
    r.host.push_back("a.com");
    r.serv.push_back("default");
    c.rules.push_back(r);

    std::cout << c << std::endl;

    conf_t cc;
    auto err = cc.ParseJSON(c);
    if (err) {
        std::cout << err << std::endl;
    } else {
        std::cout << cc << std::endl;
    }
}

}  // namespace unitest
}  // namespace gx
