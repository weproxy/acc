//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/errors/errors.h"

namespace gx {
namespace url {
// uinfo_t ...
namespace xx {
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
using Userinfo = SharedPtr<xx::uinfo_t>;

// Values ...
struct Values {
    MapPtr<string, slice<string>> map_;

    Values() : map_(new Map<string, slice<string>>()) {}

    slice<string>& operator[](const string& key) { return (*map_)[key]; }
    const slice<string>& operator[](const string& key) const { return (*map_)[key]; }

    string String() const;

    const string& Get(const string& key);
    void Set(const string& key, const string& value);
    void Add(const string& key, const string& value);
    void Del(const string& key);
    bool Has(const string& key);
    string Encode();
};

// ParseQuery ...
R<Values, error> ParseQuery(const string& query);

// url_t ...
namespace xx {
struct url_t {
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

    error setPath(const string& p);
    error setFragment(const string& f);

    string EscapedPath() const;
    string EscapedFragment() const;

    string String() const;

    bool IsAbs() const { return !Scheme.empty(); }

    Values Query();

    string RequestURI() const;
    string Hostname() const;
    string Port() const;
};
}  // namespace xx

// URL ...
using URL = SharedPtr<xx::url_t>;

}  // namespace url
}  // namespace gx

////////////////////////////////////////////////////////////////////////////////////////////////////
#include "xx.h"
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace gx {
namespace url {

// EscapeError ...
inline error EscapeError(const string& s) { return errors::New("invalid URL escape \"%s\"", s.c_str()); }

// InvalidHostError ...
inline error InvalidHostError(const string& s) {
    return errors::New("invalid character \"%s\" in host name", s.c_str());
}

// User ...
inline Userinfo User(const string& username) { return Userinfo(new xx::uinfo_t(username, "", false)); }

// UserPassword ...
inline Userinfo UserPassword(const string& username, const string& password) {
    return Userinfo(new xx::uinfo_t(username, password, true));
}

// Parse ...
R<URL, error> Parse(const string& rawURL);

// ParseRequestURI ...
R<URL, error> ParseRequestURI(const string& rawURL);

// QueryEscape ...
inline string QueryEscape(const string& s) { return xx::escape(s, xx::encodeQueryComponent); }

// QueryUnescape ...
inline R<string, error> QueryUnescape(const string& s) { return xx::unescape(s, xx::encodeQueryComponent); }

// PathEscape ...
inline string PathEscape(const string& s) { return xx::escape(s, xx::encodePathSegment); }

// PathUnescape ...
inline R<string, error> PathUnescape(const string& s) { return xx::unescape(s, xx::encodePathSegment); }

}  // namespace url
}  // namespace gx

namespace gx {
namespace unitest {
void test_strings();
}  // namespace unitest
}  // namespace gx
