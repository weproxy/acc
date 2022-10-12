//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

#define NAMESPACE_BEG_RULE \
    namespace app {      \
    namespace proto {    \
    namespace rule {
#define NAMESPACE_END_RULE \
    }                    \
    }                    \
    }

NAMESPACE_BEG_RULE

////////////////////////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[rule]";

// GetTCPRule ...
R<string, error> GetTCPRule(const string& host, net::Addr addr);

// GetUDPRule ...
R<string, error> GetUDPRule(const string& host, net::Addr addr);

// GetDNSRule ...
R<string, error> GetDNSRule(const string& host, net::Addr addr);

NAMESPACE_END_RULE
