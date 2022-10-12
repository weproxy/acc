//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "util.h"

namespace gx {
namespace url {

namespace xx {
// uinfo_t ...
struct uinfo_t {
    string username;
    string password;
    bool passwordSet{false};

    uinfo_t() = default;
    uinfo_t(const string& u, const string& p, bool b) : username(u), password(p), passwordSet(b) {}

    const string& Username() const { return username; }
    R<const string&, bool> Password() const { return {password, passwordSet}; }

    string String() const;
};
}  // namespace xx

// Userinfo ...
typedef std::shared_ptr<xx::uinfo_t> Userinfo;

// User ...
inline Userinfo User(const string& username) { return Userinfo(new xx::uinfo_t(username, "", false)); }

// UserPassword ...
inline Userinfo User(const string& username, const string& password) {
    return Userinfo(new xx::uinfo_t(username, password, true));
}

namespace xx {
struct uri_t {
    string Scheme;
    string Opaque;           // encoded opaque data
    Userinfo User;           // username and password information
    string Host;             // host or host:port
    string Path;             // path (relative paths may omit leading slash)
    string RawPath;          // encoded path hint (see EscapedPath method)
    bool OmitHost{false};    // do not emit empty host (authority)
    bool ForceQuery{false};  // append a query ('?') even if RawQuery is empty
    string RawQuery;         // encoded query values, without '?'
    string Fragment;         // fragment for references, without '#'
    string RawFragment;      // encoded fragment hint (see EscapedFragment method)
};
}  // namespace xx

// URI ...
typedef std::shared_ptr<xx::uri_t> URI;

// Parse ...
R<URI, error> Parse(const string& rawURL);

// QueryEscape escapes the string so it can be safely placed
// inside a URL query.
inline string QueryEscape(const string& s) { return xx::escape(s, xx::encodeQueryComponent); }

// PathEscape escapes the string so it can be safely placed inside a URL path segment,
// replacing special characters (including /) with %XX sequences as needed.
inline string PathEscape(const string& s) { return xx::escape(s, xx::encodePathSegment); }

}  // namespace url
}  // namespace gx
