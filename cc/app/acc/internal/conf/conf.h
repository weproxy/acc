//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"

namespace internal {
namespace conf {

// Auth ...
struct Auth {
    string s5;
    string ss;
};

// Server ...
struct Server {
    Auth auth;
    strvec dns;
    strvec main;
    Map<string, strvec> geo;
};

// Rule ...
struct Rule {
    strvec host;
    strvec serv;
};

// Conf ...
struct Conf {
    Server server;
    Vec<Rule> dns;
    Vec<Rule> rules;

    // String ...
    string String() const;
    operator string() const { return String(); }

    // ParseJSON ...
    error ParseJSON(const string& jsonContent);

   private:
    // Fix ...
    error Fix();
};

////////////////////////////////////////////////////////////////////////////////
//

// NewFromJSON ...
R<Ref<Conf>, error> NewFromJSON(const string& jsonContent);

// Default ...
Ref<Conf> Default();

}  // namespace conf
}  // namespace internal
