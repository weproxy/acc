//
// weproxy@foxmail.com 2022/10/03
//

#include "url.h"

#include "gx/fmt/fmt.h"
#include "gx/strings/strings.h"

namespace gx {
namespace url {

// Paarse ...
R<Ref<URL>, error> Parse(const string& rawURL) {
    AUTO_R(u, frag, _, strings::Cut(rawURL, "#"));
    AUTO_R(url, err, xx::parse(u, false));
    if (err) {
        return {nil, errors::New(GX_SS("parse " << u << " " << err))};
    }
    if (frag == "") {
        return {url, nil};
    }
    err = url->setFragment(frag);
    if (err) {
        return {nil, errors::New(GX_SS("parse " << rawURL << " " << err))};
    }

    return {url, nil};
}

// ParseRequestURI ...
R<Ref<URL>, error> ParseRequestURI(const string& rawURL) {
    AUTO_R(url, err, xx::parse(rawURL, true));
    if (err) {
        return {nil, errors::New(GX_SS("parse " << rawURL << " " << err))};
    }
    return {url, nil};
}

////////////////////////////////////////////////////////////////////////////////
//
// String ...
string Userinfo::String() const {
    auto s = escape(username, xx::encodeUserPassword);
    if (passwordSet) {
        s += ":" + xx::escape(password, xx::encodeUserPassword);
    }
    return s;
}

// EscapedPath ...
string URL::EscapedPath() const {
    auto& u = *this;
    if (u.RawPath != "" && xx::validEncoded(u.RawPath, xx::encodePath)) {
        AUTO_R(p, err, xx::unescape(u.RawPath, xx::encodePath));
        if (err == nil && p == u.Path) {
            return u.RawPath;
        }
    }
    if (u.Path == "*") {
        return "*";  // don't escape (Issue 11202)
    }
    return xx::escape(u.Path, xx::encodePath);
}

// EscapedFragment ...
string URL::EscapedFragment() const {
    auto& u = *this;
    if (u.RawFragment != "" && xx::validEncoded(u.RawFragment, xx::encodeFragment)) {
        AUTO_R(f, err, xx::unescape(u.RawFragment, xx::encodeFragment));
        if (err == nil && f == u.Fragment) {
            return u.RawFragment;
        }
    }
    return xx::escape(u.Fragment, xx::encodeFragment);
}

// String ...
string URL::String() const {
    auto& u = *this;
    strings::Builder buf;

    if (u.Scheme != "") {
        buf.WriteString(u.Scheme);
        buf.WriteByte(':');
    }

    if (u.Opaque != "") {
        buf.WriteString(u.Opaque);
    } else {
        if (u.Scheme != "" || u.Host != "" || u.User != nil) {
            if (u.OmitHost && u.Host == "" && u.User == nil) {
                // omit empty host
            } else {
                if (u.Host != "" || u.Path != "" || u.User != nil) {
                    buf.WriteString("//");
                }
                if (u.User != nil) {
                    buf.WriteString(u.User->String());
                    buf.WriteByte('@');
                }
                if (u.Host != "") {
                    buf.WriteString(xx::escape(u.Host, xx::encodeHost));
                }
            }
        }
        string path = u.EscapedPath();
        if (path != "" && path[0] != '/' && u.Host != "") {
            buf.WriteByte('/');
        }
        if (buf.Len() == 0) {
            AUTO_R(segment, _1, _2, strings::Cut(path, "/"));
            if (strings::Contains(segment, ":")) {
                buf.WriteString("./");
            }
        }
        buf.WriteString(path);
    }
    if (u.ForceQuery || u.RawQuery != "") {
        buf.WriteByte('?');
        buf.WriteString(u.RawQuery);
    }
    if (u.Fragment != "") {
        buf.WriteByte('#');
        buf.WriteString(u.EscapedFragment());
    }

    return buf.String();
}

// Query ...
Values URL::Query() {
    AUTO_R(v, _, ParseQuery(this->RawQuery));
    return v;
}

// RequestURI ...
string URL::RequestURI() const {
    auto& u = *this;
    string result = u.Opaque;
    if (result == "") {
        result = u.EscapedPath();
        if (result == "") {
            result = "/";
        }
    } else {
        if (strings::HasPrefix(result, "//")) {
            result = u.Scheme + ":" + result;
        }
    }
    if (u.ForceQuery || u.RawQuery != "") {
        result += "?" + u.RawQuery;
    }
    return result;
}

// Hostname ...
string URL::Hostname() const {
    AUTO_R(host, _, xx::splitHostPort(this->Host));
    return host;
}

// Port ...
string URL::Port() const {
    AUTO_R(_, port, xx::splitHostPort(this->Host));
    return port;
}

// setPath ...
error URL::setPath(const string& p) {
    AUTO_R(path, err, xx::unescape(p, xx::encodePath));
    if (err != nil) {
        return err;
    }
    auto& u = *this;

    u.Path = path;
    string escp = xx::escape(path, xx::encodePath);
    if (p == escp) {
        // Default encoding is fine.
        u.RawPath = "";
    } else {
        u.RawPath = p;
    }
    return nil;
}

// setFragment ...
error URL::setFragment(const string& f) {
    AUTO_R(frag, err, xx::unescape(f, xx::encodeFragment));
    if (err != nil) {
        return err;
    }

    auto& u = *this;

    u.Fragment = frag;
    string escf = xx::escape(frag, xx::encodeFragment);
    if (f == escf) {
        // Default encoding is fine.
        u.RawFragment = "";
    } else {
        u.RawFragment = f;
    }
    return nil;
}

namespace xx {
// parseQuery ...
static error parseQuery(Values m, const string& query1) {
    string query(query1);
    error err;

    for (; query != "";) {
        AUTO_R(key, _query, _, strings::Cut(query, "&"));
        query = _query;
        if (strings::Contains(key, ";")) {
            err = fmt::Errorf("invalid semicolon separator in query");
            continue;
        }
        if (key == "") {
            continue;
        }
        AUTO_R(_key, value, _1, strings::Cut(key, "="));
        AUTO_R(_key2, err1, QueryUnescape(_key));
        key = _key2;
        if (err1 != nil) {
            if (err == nil) {
                err = err1;
            }
            continue;
        }
        AUTO_R(_value, err2, QueryUnescape(value));
        value = _value;
        if (err2 != nil) {
            if (err == nil) {
                err = err2;
            }
            continue;
        }
        m[key] = append(m[key], value);
    }
    return err;
}

}  // namespace xx

// ParseQuery ...
R<Values, error> ParseQuery(const string& query) {
    Values m;
    auto err = xx::parseQuery(m, query);
    return {m, err};
}

// Encode encodes the values into “URL encoded” form
// ("bar=baz&foo=quux") sorted by key.
string Values::Encode() const {
    if (!map_) {
        return "";
    }
    strings::Builder buf;
    slice<string> keys = make<string>(0, len(map_));
    for (auto& kv : *map_) {
        keys = append(keys, kv.first);
    }
    // sort.Strings(keys)
    // for _, k := range keys {
    for (int i = 0; i < len(keys); i++) {
        auto k = keys[i];
        auto& vs = map_[k];
        auto keyEscaped = QueryEscape(k);
        for (int j = 0; j < len(vs); j++) {
            auto v = vs[j];
            if (buf.Len() > 0) {
                buf.WriteByte('&');
            }
            buf.WriteString(keyEscaped);
            buf.WriteByte('=');
            buf.WriteString(QueryEscape(v));
        }
    }
    return buf.String();
}

}  // namespace url
}  // namespace gx

namespace gx {
namespace unitest {

#ifndef LOGS_D
#define LOGS_D(...) std::cout << __VA_ARGS__ << std::endl;
#endif

void test_net_url() {
    const char* rawURL = "https://user:pass@host.com:8909/a/b?p1=a&p2=b#1234";

    AUTO_R(uri, err, url::Parse(rawURL));
    if (err) {
        LOGS_D("err: " << err);
        return;
    }

    LOGS_D("raw: " << rawURL);
    LOGS_D("uri: " << uri);
}
}  // namespace unitest
}  // namespace gx
