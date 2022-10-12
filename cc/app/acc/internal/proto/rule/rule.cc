//
// weproxy@foxmail.com 2022/10/03
//

#include "rule.h"

NAMESPACE_BEG_RULE

// _errRuleNotFound ...
static const error _errRuleNotFound = errors::New("rule not found");

// GetTCPRule ...
R<string, error> GetTCPRule(const string& host, net::Addr addr) {
    // TODO ...
    return {"", _errRuleNotFound};
}

// GetUDPRule ...
R<string, error> GetUDPRule(const string& host, net::Addr addr) {
    // TODO ...
    return {"", _errRuleNotFound};
}

// GetDNSRule ...
R<string, error> GetDNSRule(const string& host, net::Addr addr) {
    // TODO ...
    return {"", _errRuleNotFound};
}

NAMESPACE_END_RULE
