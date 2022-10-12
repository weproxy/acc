//
// weproxy@foxmail.com 2022/10/03
//

#include "url.h"

namespace gx {
namespace url {

namespace xx {
string uinfo_t::String() const { return ""; }
}  // namespace xx

// Paarse ...
R<URI, error> Parse(const string& rawURL) { return {nil, errors::New("fail")}; }

}  // namespace url
}  // namespace gx
