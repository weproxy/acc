//
// weproxy@foxmail.com 2022/10/03
//

#include "util.h"

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
        for (int i  = 0; i < t.length(); i++) {
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
    for (int i  = 0; i < s.length(); i++) {
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
            case '+':
                hasPlus = mode == encodeQueryComponent;
                i++;
            default:
                if ((mode == encodeHost || mode == encodeZone) && s[i] < 0x80 && shouldEscape(s[i], mode)) {
                    return {"", InvalidHostError(s.substr(i, i + 1))};
                }
                i++;
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
            case '+':
                if (mode == encodeQueryComponent) {
                    t += ' ';
                } else {
                    t += '+';
                }
            default:
                t += (char)s[i];
        }
    }
    return {t, nil};
}

}  // namespace xx

}  // namespace url
}  // namespace gx
