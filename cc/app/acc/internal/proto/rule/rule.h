//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../proto.h"

namespace internal {
namespace proto {
namespace rule {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[rule]";

// GetTCPRule ...
R<string, error> GetTCPRule(const string& host, net::Addr addr);

// GetUDPRule ...
R<string, error> GetUDPRule(const string& host, net::Addr addr);

// GetDNSRule ...
R<string, error> GetDNSRule(const string& host, net::Addr addr);

}  // namespace rule
}  // namespace proto
}  // namespace internal
