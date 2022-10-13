//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/builtin/builtin.h"
#include "gx/errors/errors.h"

namespace gx {
namespace url {

// EscapeError ...
inline error EscapeError(const string& s) { return errors::New("invalid URL escape \"%s\"", s.c_str()); }

// InvalidHostError ...
inline error InvalidHostError(const string& s) {
    return errors::New("invalid character \"%s\" in host name", s.c_str());
}

namespace xx {
// encoding ...
enum encoding {
    encodePath = 1,
    encodePathSegment,
    encodeHost,
    encodeZone,
    encodeUserPassword,
    encodeQueryComponent,
    encodeFragment,
};

// shouldEscape ...
bool shouldEscape(byte c, encoding mode);

// escape ...
string escape(const string& s, encoding mode);

// unescape ...
R<string, error> unescape(const string& s, encoding mode);

// getScheme ...
R<string/*scheme*/, string/*path*/, error> getScheme(const string& rawURL );

}  // namespace xx

}  // namespace url
}  // namespace gx
