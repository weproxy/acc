//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/errors/errors.h"

namespace gx {
namespace url {
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
R<string /*scheme*/, string /*path*/, error> getScheme(const string& rawURL);

// validEncoded ...
bool validEncoded(const string& s, encoding mode);

// validUserinfo ...
bool validUserinfo(const string& s);

// validOptionalPort ...
bool validOptionalPort(const string& port);

// stringContainsCTLByte ...
bool stringContainsCTLByte(const string& s);

// parse ...
R<Ref<URL>, error> parse(const string& rawURL, bool viaRequest);

// parseAuthority ...
R<Ref<Userinfo>, string, error> parseAuthority(const string& authority);

// parseHost ...
R<string, error> parseHost(const string& host);

// splitHostPort ...
R<string, string> splitHostPort(const string& hostPort);

}  // namespace xx
}  // namespace url
}  // namespace gx
