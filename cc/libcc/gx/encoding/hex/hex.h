//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "builtin/builtin.h"

namespace gx {
namespace hex {

// EncodeToString ...
string EncodeToString(const void* src, size_t len);
inline string EncodeToString(const byte_s& s) { return EncodeToString(s.data(), s.length()); }
inline string EncodeToString(const string& s) { return EncodeToString(s.data(), s.length()); }

// DecodeString ...
R<Vec<byte>, error> DecodeString(const string& s);

}  // namespace hex
}  // namespace gx

namespace gx {
namespace unitest {
void test_hex();
}  // namespace unitest
}  // namespace gx
