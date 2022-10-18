//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"

namespace app {
namespace conf {
namespace xx {

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

}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
//

// Conf ...
using Conf = Ref<xx::conf_t>;

// NewFromJSON ...
R<Conf, error> NewFromJSON(const string& jsonContent);

// Default ...
Conf Default();

}  // namespace conf
}  // namespace app
