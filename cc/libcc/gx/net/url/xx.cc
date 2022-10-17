//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/fmt/fmt.h"
#include "gx/strings/strings.h"
#include "url.h"

namespace gx {
namespace url {
namespace xx {

static const char* upperhex = "0123456789ABCDEF";

static bool ishex(byte c) {
    if ('0' <= c && c <= '9') {
        return true;
    } else if ('a' <= c && c <= 'f') {
        return true;
    } else if ('A' <= c && c <= 'F') {
        return true;
    }
    return false;
}

static byte unhex(byte c) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else if ('a' <= c && c <= 'f') {
        return c - 'a' + 10;
    } else if ('A' <= c && c <= 'F') {
        return c - 'A' + 10;
    }
    return 0;
}

// shouldEscape ...
bool shouldEscape(byte c, encoding mode) {
    if ('a' <= c && c <= 'z' || 'A' <= c && c <= 'Z' || '0' <= c && c <= '9') {
        return false;
    }

    if (mode == encodeHost || mode == encodeZone) {
        switch (c) {
            case '!':
            case '$':
            case '&':
            case '\'':
            case '(':
            case ')':
            case '*':
            case '+':
            case ',':
            case ';':
            case '=':
            case ':':
            case '[':
            case ']':
            case '<':
            case '>':
            case '"':
                return false;
        }
    }

    switch (c) {
        case '-':
        case '_':
        case '.':
        case '~':  // §2.3 Unreserved characters (mark)
            return false;

        case '$':
        case '&':
        case '+':
        case ',':
        case '/':
        case ':':
        case ';':
        case '=':
        case '?':
        case '@':  // §2.2 Reserved characters (reserved)
            switch (mode) {
                case encodePath:  // §3.3
                    return c == '?';
                case encodePathSegment:  // §3.3
                    return c == '/' || c == ';' || c == ',' || c == '?';
                case encodeUserPassword:  // §3.2.1
                    return c == '@' || c == '/' || c == '?' || c == ':';
                case encodeQueryComponent:  // §3.4
                    // The RFC reserves (so we must escape) everything.
                    return true;
                case encodeFragment:  // §4.1
                    return false;
            }
            break;
    }

    if (mode == encodeFragment) {
        switch (c) {
            case '!':
            case '(':
            case ')':
            case '*':
                return false;
        }
    }

    // Everything else must be escaped.
    return true;
}

// escape ...
string escape(const string& s, encoding mode) {
    int spaceCount = 0, hexCount = 0;
    for (int i = 0; i < s.length(); i++) {
        char c = s[i];
        if (shouldEscape(c, mode)) {
            if (c == ' ' && mode == encodeQueryComponent) {
                spaceCount++;
            } else {
                hexCount++;
            }
        }
    }

    if (spaceCount == 0 && hexCount == 0) {
        return s;
    }

    if (hexCount == 0) {
        string t(s);
        for (int i = 0; i < t.length(); i++) {
            if (t[i] == ' ') {
                t[i] = '+';
            }
        }
        return t;
    }

    int required = s.length() + 2 * hexCount;

    string t;
    t.reserve(required + 1);

    int j = 0;
    for (int i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c == ' ' && mode == encodeQueryComponent) {
            t[j] = '+';
            j++;
        } else if (shouldEscape(c, mode)) {
            t[j] = '%';
            t[j + 1] = upperhex[c >> 4];
            t[j + 2] = upperhex[c & 15];
            j += 3;
        } else {
            t[j] = s[i];
            j++;
        }
    }
    return string(t.c_str(), j);
}

// unescape ...
R<string, error> unescape(const string& ss, encoding mode) {
    string s(ss);

    int n = 0;
    bool hasPlus = false;
    for (int i = 0; i < s.length();) {
        switch (s[i]) {
            case '%':
                n++;
                if (i + 2 >= s.length() || !ishex(s[i + 1]) || !ishex(s[i + 2])) {
                    return {"", EscapeError(s.substr(i, i + 3))};
                }
                if (mode == encodeHost && unhex(s[i + 1]) < 8 && memcmp(s.c_str() + i, "%25", 3) != 0) {
                    return {"", EscapeError(s.substr(i, i + 3))};
                }
                if (mode == encodeZone) {
                    auto v = unhex(s[i + 1]) << 4 | unhex(s[i + 2]);
                    if (memcmp(s.c_str() + i, "%25", 3) != 0 && v != ' ' && shouldEscape(v, encodeHost)) {
                        return {"", EscapeError(s.substr(i, i + 3))};
                    }
                }
                i += 3;
                break;
            case '+':
                hasPlus = mode == encodeQueryComponent;
                i++;
                break;
            default:
                if ((mode == encodeHost || mode == encodeZone) && s[i] < 0x80 && shouldEscape(s[i], mode)) {
                    return {"", InvalidHostError(s.substr(i, i + 1))};
                }
                i++;
                break;
        }
    }

    if (n == 0 && !hasPlus) {
        return {s, nil};
    }

    string t;
    t.reserve(s.length() - 2 * n);
    for (int i = 0; i < s.length(); i++) {
        switch (s[i]) {
            case '%':
                t += (char)(unhex(s[i + 1]) << 4 | unhex(s[i + 2]));
                i += 2;
                break;
            case '+':
                if (mode == encodeQueryComponent) {
                    t += ' ';
                } else {
                    t += '+';
                }
                break;
            default:
                t += (char)s[i];
                break;
        }
    }
    return {t, nil};
}

// getScheme ...
R<string /*scheme*/, string /*path*/, error> getScheme(const string& rawURL) {
    for (int i = 0; i < len(rawURL); i++) {
        char c = rawURL[i];
        if ('a' <= c && c <= 'z' || 'A' <= c && c <= 'Z') {
            // do nothing
        } else if ('0' <= c && c <= '9' || c == '+' || c == '-' || c == '.') {
            if (i == 0) {
                return {"", rawURL, nil};
            }
        } else if (c == ':') {
            if (i == 0) {
                return {"", "", errors::New("missing protocol scheme")};
            }
            return {rawURL.substr(0, i), rawURL.substr(i + 1), nil};
        } else {
            // we have encountered an invalid character,
            // so there is no valid scheme
            return {"", rawURL, nil};
        }
    }
    return {"", rawURL, nil};
}

// validEncoded ...
bool validEncoded(const string& s, encoding mode) {
    for (int i = 0; i < len(s); i++) {
        // RFC 3986, Appendix A.
        // pchar = unreserved / pct-encoded / sub-delims / ":" / "@".
        // shouldEscape is not quite compliant with the RFC,
        // so we check the sub-delims ourselves and let
        // shouldEscape handle the others.
        switch (s[i]) {
            case '!':
            case '$':
            case '&':
            case '\'':
            case '(':
            case ')':
            case '*':
            case '+':
            case ',':
            case ';':
            case '=':
            case ':':
            case '@':
                // ok
            case '[':
            case ']':
                // ok - not specified in RFC 3986 but left alone by modern browsers
            case '%':
                // ok - percent encoded, will decode
                break;
            default:
                if (shouldEscape(s[i], mode)) {
                    return false;
                }
        }
    }
    return true;
}

// validUserinfo ...
bool validUserinfo(const string& s) {
    for (auto r : s) {
        if (('A' <= r && r <= 'Z') || ('a' <= r && r <= 'z') || ('0' <= r && r <= '9')) {
            continue;
        }
        switch (r) {
            case '-':
            case '.':
            case '_':
            case ':':
            case '~':
            case '!':
            case '$':
            case '&':
            case '\'':
            case '(':
            case ')':
            case '*':
            case '+':
            case ',':
            case ';':
            case '=':
            case '%':
            case '@':
                continue;
            default:
                return false;
        }
    }
    return true;
}
// validOptionalPort ...
bool validOptionalPort(const string& port) {
    if (port.empty()) {
        return true;
    }
    if (port[0] != ':') {
        return false;
    }

    for (int i = 1; i < port.length(); i++) {
        char b = port[i];
        if (b < '0' || b > '9') {
            return false;
        }
    }
    return true;
}

// stringContainsCTLByte ...
bool stringContainsCTLByte(const string& s) {
    for (int i = 0; i < len(s); i++) {
        byte b = s[i];
        if (b < ' ' || b == 0x7f) {
            return true;
        }
    }
    return false;
}

// parse ...
R<URL, error> parse(const string& rawURL, bool viaRequest) {
    string rest;
    error err;

    if (stringContainsCTLByte(rawURL)) {
        return {nil, errors::New("net/url: invalid control character in URL")};
    }

    if (rawURL == "" && viaRequest) {
        return {nil, errors::New("empty url")};
    }

    URL url = SharedPtr<url_t>(new url_t());

    if (rawURL == "*") {
        url->Path = "*";
        return {url, nil};
    }

    // Split off possible leading "http:", "mailto:", etc.
    // Cannot contain escaped characters.
    AUTO_R(_scheme, _rest, _err, getScheme(rawURL));
    url->Scheme = _scheme;
    rest = _rest;
    if (_err != nil) {
        return {nil, _err};
    }
    url->Scheme = strings::ToLower(url->Scheme);

    if (strings::HasSuffix(rest, "?") && strings::Count(rest, "?") == 1) {
        url->ForceQuery = true;
        rest = rest.substr(0, len(rest) - 1);
    } else {
        AUTO_R(_rest, _rawQueryy, _, strings::Cut(rest, "?"));
        rest = _rest;
        url->RawQuery = _rawQueryy;
    }

    if (!strings::HasPrefix(rest, "/")) {
        if (url->Scheme != "") {
            // We consider rootless paths per RFC 3986 as opaque.
            url->Opaque = rest;
            return {url, nil};
        }
        if (viaRequest) {
            return {nil, errors::New("invalid URL for request")};
        }

        // Avoid confusion with malformed schemes, like cache_object:foo/bar.
        // See golang.org/issue/16822.
        //
        // RFC 3986, §3.3:
        // In addition, a URL reference (Section 4.1) may be a relative-path reference,
        // in which case the first path segment cannot contain a colon (":") character.
        AUTO_R(segment, _1, _2, strings::Cut(rest, "/"));
        if (strings::Contains(segment, ":")) {
            // First path segment has colon. Not allowed in relative URL.
            return {nil, errors::New("first path segment in URL cannot contain colon")};
        }
    }

    if ((url->Scheme != "" || !viaRequest && !strings::HasPrefix(rest, "///")) && strings::HasPrefix(rest, "//")) {
        string authority = rest.substr(2);
        rest = "";
        int i = strings::Index(authority, "/");
        if (i >= 0) {
            rest = authority.substr(i);
            authority = authority.substr(0, i);
        }
        AUTO_R(_user, _host, err, parseAuthority(authority));
        url->User = _user;
        url->Host = _host;
        if (err != nil) {
            return {nil, err};
        }
    } else if (url->Scheme != "" && strings::HasPrefix(rest, "/")) {
        // OmitHost is set to true when rawURL has an empty host (authority).
        // See golang.org/issue/46059.
        url->OmitHost = true;
    }

    // Set Path and, optionally, RawPath.
    // RawPath is a hint of the encoding of Path. We don't want to set it if
    // the default escaping of Path is equivalent, to help make sure that people
    // don't rely on it in general.
    err = url->setPath(rest);
    if (err != nil) {
        return {nil, err};
    }
    return {url, nil};
}

// parseAuthority ...
R<Userinfo, string, error> parseAuthority(const string& authority) {
    string host;
    error err;

    int i = strings::LastIndex(authority, "@");
    if (i < 0) {
        AUTO_R(_host, _err, parseHost(authority));
        host = _host;
        err = _err;
    } else {
        AUTO_R(_host, _err, parseHost(authority.substr(i + 1)));
        host = _host;
        err = _err;
    }

    if (err != nil) {
        return {nil, "", err};
    }
    if (i < 0) {
        return {nil, host, nil};
    }
    Userinfo user;
    string userinfo = authority.substr(0, i);
    if (!validUserinfo(userinfo)) {
        return {nil, "", errors::New("net/url: invalid userinfo")};
    }
    if (!strings::Contains(userinfo, ":")) {
        AUTO_R(userinfo, err, unescape(userinfo, encodeUserPassword));
        if (err != nil) {
            return {nil, "", err};
        }
        user = User(userinfo);
    } else {
        AUTO_R(_username, _password, _, strings::Cut(userinfo, ":"));
        AUTO_R(username, err, unescape(_username, encodeUserPassword));
        if (err != nil) {
            return {nil, "", err};
        }
        AUTO_R(password, er2, unescape(_password, encodeUserPassword));
        if (er2 != nil) {
            return {nil, "", er2};
        }
        user = UserPassword(username, password);
    }
    return {user, host, nil};
}

// parseHost ...
R<string, error> parseHost(const string& host) {
    if (strings::HasPrefix(host, "[")) {
        // Parse an IP-Literal in RFC 3986 and RFC 6874.
        // E.g., "[fe80::1]", "[fe80::1%25en0]", "[fe80::1]:80".
        int i = strings::LastIndex(host, "]");
        if (i < 0) {
            return {"", errors::New("missing ']' in host")};
        }
        string colonPort = host.substr(i + 1);
        if (!validOptionalPort(colonPort)) {
            return {"", fmt::Errorf("invalid port %s after host", colonPort.c_str())};
        }

        // RFC 6874 defines that %25 (%-encoded percent) introduces
        // the zone identifier, and the zone identifier can use basically
        // any %-encoding it likes. That's different from the host, which
        // can only %-encode non-ASCII bytes.
        // We do impose some restrictions on the zone, to avoid stupidity
        // like newlines.
        int zone = strings::Index(host.substr(0, i), "%25");
        if (zone >= 0) {
            AUTO_R(host1, err, unescape(host.substr(0, zone), encodeHost));
            if (err != nil) {
                return {"", err};
            }
            AUTO_R(host2, er2, unescape(host.substr(zone, i), encodeZone));
            if (er2 != nil) {
                return {"", er2};
            }
            AUTO_R(host3, er3, unescape(host.substr(i), encodeHost));
            if (er3 != nil) {
                return {"", er3};
            }
            return {host1 + host2 + host3, nil};
        }
    } else {
        int i = strings::LastIndex(host, ":");
        if (i != -1) {
            string colonPort = host.substr(i);
            if (!validOptionalPort(colonPort)) {
                return {"", fmt::Errorf("invalid port %s after host", colonPort.c_str())};
            }
        }
    }

    AUTO_R(_host, err, unescape(host, encodeHost));
    if (err != nil) {
        return {"", err};
    }
    return {_host, nil};
}

// setPath ...
error url_t::setPath(const string& p) {
    AUTO_R(path, err, unescape(p, encodePath));
    if (err != nil) {
        return err;
    }
    auto& u = *this;

    u.Path = path;
    string escp = escape(path, encodePath);
    if (p == escp) {
        // Default encoding is fine.
        u.RawPath = "";
    } else {
        u.RawPath = p;
    }
    return nil;
}

// setFragment ...
error url_t::setFragment(const string& f) {
    AUTO_R(frag, err, unescape(f, encodeFragment));
    if (err != nil) {
        return err;
    }

    auto& u = *this;

    u.Fragment = frag;
    string escf = escape(frag, encodeFragment);
    if (f == escf) {
        // Default encoding is fine.
        u.RawFragment = "";
    } else {
        u.RawFragment = f;
    }
    return nil;
}

// splitHostPort ...
R<string, string> splitHostPort(const string& hostPort) {
    string host = hostPort, port;

    int colon = strings::LastIndexByte(host, ':');
    if (colon != -1 && validOptionalPort(host.substr(colon))) {
        port = host.substr(colon + 1);
        host = host.substr(0, colon);
    }

    if (strings::HasPrefix(host, "[") && strings::HasSuffix(host, "]")) {
        host = host.substr(1, len(host) - 1);
    }

    return {host, port};
}

}  // namespace xx
}  // namespace url
}  // namespace gx
