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

    error CompileRegexp();
    bool IsMatch(const string& s);
    string GetServ();
};

// Conf ...
struct Conf {
    Server server;
    Vec<Rule> dns;
    Vec<Rule> rules;

    // String ...
    string String() const;
    operator string() const { return String(); }

    // Parse ...
    error Parse(const string& js);

    string GetMain();
    string GetDNS();

    R<Ref<Rule>, int> GetRule(const string& s);
    R<Ref<Rule>, int> GetDNSRule(const string& s);

   private:
    // fix ...
    error fix();
};

////////////////////////////////////////////////////////////////////////////////
//

// MustGet ...
Ref<Conf> MustGet(const string& key);

// Parse ...
R<Ref<Conf>, error> Parse(const string& key, const string& js);

// ParseBytes ...
R<Ref<Conf>, error> ParseBytes(const string& key, const bytez<>& js);

// ParseFile ...
R<Ref<Conf>, error> ParseFile(const string& key, const string& jsfile);

// Default ...
Ref<Conf> Default();

}  // namespace conf
}  // namespace internal
