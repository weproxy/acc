//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/errors/errors.h"
#include "gx/fmt/fmt.h"

namespace gx {
namespace url {

// Userinfo ...
struct Userinfo {
    string username;
    string password;
    bool passwordSet{false};

    Userinfo() = default;
    Userinfo(const string& u, const string& p, bool b) : username(u), password(p), passwordSet(b) {}

    const string& Username() const { return username; }
    R<const string&, bool> Password() const { return {password, passwordSet}; }

    string String() const;
};

// Values ...
struct Values {
    using K = string;
    using V = slice<string>;

    map<K, V> map_;

    V& operator[](const K& key) { return map_[key]; }
    const V& operator[](const K& key) const { return map_[key]; }

    string String() const { return GX_SS(map_); }

    // Get ...
    string Get(const K& key) const {
        AUTO_R(vs, ok, map_(key));
        if (!ok || len(vs) == 0) {
            return "";
        }
        return vs[0];
    }

    // Set ...
    void Set(const K& key, const string& val) {
        if (map_) {
            map_[key] = V{val};
        }
    }

    // Add ...
    void Add(const K& key, const string& val) {
        if (map_) {
            map_[key] = append(map_[key], val);
        }
    }

    // Del ...
    void Del(const K& key) {
        if (map_) {
            map_.Del(key);
        }
    }

    // Has ...
    bool Has(const K& key) const {
        AUTO_R(_, ok, map_(key));
        return ok;
    }

    // Encode encodes the values into “URL encoded” form
    // ("bar=baz&foo=quux") sorted by key.
    string Encode() const;
};

// ParseQuery ...
R<Values, error> ParseQuery(const string& query);

// URL ...
struct URL {
    string Scheme;
    string Opaque;           // encoded opaque data
    Ref<Userinfo> User;      // username and password information
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

}  // namespace url
}  // namespace gx

////////////////////////////////////////////////////////////////////////////////////////////////////
#include "xx.h"
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace gx {
namespace url {

// EscapeError ...
inline error EscapeError(const string& s) { return fmt::Errorf("invalid URL escape \"%s\"", s.c_str()); }

// InvalidHostError ...
inline error InvalidHostError(const string& s) {
    return fmt::Errorf("invalid character \"%s\" in host name", s.c_str());
}

// User ...
inline Ref<Userinfo> User(const string& username) { return NewRef<Userinfo>(username, "", false); }

// UserPassword ...
inline Ref<Userinfo> UserPassword(const string& username, const string& password) {
    return NewRef<Userinfo>(username, password, true);
}

// Parse ...
R<Ref<URL>, error> Parse(const string& rawURL);

// ParseRequestURI ...
R<Ref<URL>, error> ParseRequestURI(const string& rawURL);

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
void test_net_url();
}  // namespace unitest
}  // namespace gx
