//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"
#include "builder.h"

namespace gx {
namespace strings {

// IndexByte ...
inline int IndexByte(const string& s, byte c) {
    for (int i = 0; i < s.length(); i++) {
        if (s[i] == c) {
            return i;
        }
    }
    return -1;
}

// Index ...
inline int Index(const string& s, const string& substr) {
    int sl = s.length(), rl = substr.length();

    if (rl == 0) {
        return 0;
    } else if (rl == 1) {
        return IndexByte(s, substr[0]);
    } else if (rl == sl) {
        return s == substr ? 0 : -1;
    }

    const char *a = s.c_str(), *b = substr.c_str();
    for (int i = 0; i < sl - rl; i++) {
        if (memcmp(a + i, b, rl) == 0) {
            return i;
        }
    }

    return -1;
}

// LastIndexByte ...
inline int LastIndexByte(const string& s, byte c) {
    for (int i = s.length() - 1; i >= 0; i--) {
        if (s[i] == c) {
            return i;
        }
    }
    return -1;
}

// LastIndex ...
inline int LastIndex(const string& s, const string& substr) {
    int sl = s.length(), rl = substr.length();

    if (rl == 0) {
        return 0;
    } else if (rl == 1) {
        return LastIndexByte(s, substr[0]);
    } else if (rl == sl) {
        return s == substr ? 0 : -1;
    }

    const char *a = s.c_str(), *b = substr.c_str();
    for (int i = sl - rl; i >= 0; i--) {
        if (memcmp(a + i, b, rl) == 0) {
            return i;
        }
    }
    return -1;
}

// Count ...
inline int Count(const string& s, const string& substr) {
    int cnt = 0;
    int sl = s.length(), rl = substr.length();
    const char *a = s.c_str(), *b = substr.c_str();
    for (int i = 0; i < sl - rl;) {
        if (memcmp(a + i, b, rl) == 0) {
            cnt++;
            i += rl;
        } else {
            i++;
        }
    }
    return cnt;
}

// Contains ...
inline bool Contains(const string& s, const string& substr) { return Index(s, substr) >= 0; }

// IndexAny ...
inline int IndexAny(const string& s, const string& chars) { return Index(s, chars); }

namespace xx {
Vec<string> genSplit(const string& s, const string& sep, int sepSave, int n);
}  // namespace xx

// Split ...
inline Vec<string> SplitN(const string& s, const string& sep, int n) { return xx::genSplit(s, sep, 0, n); }

// SplitAfter ...
inline Vec<string> SplitAfterN(const string& s, const string& sep, int n) {
    return xx::genSplit(s, sep, sep.length(), n);
}

// Split ...
inline Vec<string> Split(const string& s, const string& sep) { return xx::genSplit(s, sep, 0, -1); }

// SplitAfter ...
inline Vec<string> SplitAfter(const string& s, const string& sep) { return xx::genSplit(s, sep, sep.length(), -1); }

// Fields ...
slice<string> Fields(const string& s);

// Join ...
template <typename... T>
string Join(const string& sep, T&&... s) {
    return "";
}

// Repeat ...
inline string Repeat(const string& s, int count) {
    int len = s.length() * count;
    char* buf = (char*)malloc(len);
    for (int i = 0; i < count; i++) {
        memcpy(buf + i, s.c_str(), s.length());
    }
    string t(buf, len);
    delete[] buf;
    return t;
}

namespace xx {
inline bool cutsetMatch(const string& cutset, char v) {
    for (char c : cutset) {
        if (c == v) {
            return true;
        }
    }
    return false;
}
inline int cutsetLeftIndex(const string& s, const string& cutset) {
    int i = 0;
    for (; i < s.length(); i++) {
        if (!cutsetMatch(cutset, s[i])) {
            break;
        }
    }
    return i;
}
inline int cutsetRightIndex(const string& s, const string& cutset) {
    int i = s.length() - 1;
    for (; i >= 0; i--) {
        if (!cutsetMatch(cutset, s[i])) {
            break;
        }
    }
    return i;
}
}  // namespace xx

// TrimLeft ...
inline string TrimLeft(const string& s, const string& cutset) {
    if (s.empty()) {
        return s;
    }
    return string(s.c_str() + xx::cutsetLeftIndex(s, cutset));
}

// Trim ...
inline string Trim(const string& s, const string& cutset) {
    if (s.empty()) {
        return s;
    }
    int i = xx::cutsetLeftIndex(s, cutset);
    int j = xx::cutsetRightIndex(s, cutset);
    return string(s.c_str() + i, j);
}

// TrimRight ...
inline string TrimRight(const string& s, const string& cutset) {
    if (s.empty()) {
        return s;
    }
    int i = xx::cutsetRightIndex(s, cutset);
    return i >= 0 ? string(s.c_str(), i + 1) : "";
}

// TrimSpace ...
inline string TrimSpace(const string& s) { return Trim(s, " \t\r\n"); }

// HasPrefix ...
inline bool HasPrefix(const string& s, const string& prefix) {
    return s.length() >= prefix.length() && memcmp(s.c_str(), prefix.c_str(), prefix.length()) == 0;
}

// HasSuffix ...
inline bool HasSuffix(const string& s, const string& suffix) {
    return s.length() >= suffix.length() &&
           memcmp(s.c_str() + s.length() - suffix.length(), suffix.c_str(), suffix.length()) == 0;
}

// TrimSuffix ...
inline string TrimPrefix(const string& s, const string& prefix) {
    if (HasPrefix(s, prefix)) {
        return s.substr(prefix.length());
    }
    return s;
}

// TrimSuffix ...
inline string TrimSuffix(const string& s, const string& suffix) {
    if (HasSuffix(s, suffix)) {
        return s.substr(0, s.length() - suffix.length());
    }
    return s;
}

// Replace ...
string Replace(const string& s, const string& olds, const string& news, int n);

// ReplaceAll ...
inline string ReplaceAll(const string& s, const string& olds, const string& news) { return Replace(s, olds, news, -1); }

// EqualFold ...
inline bool EqualFold(const string& s, const string& t) {
    if (&s == &t) {
        return true;
    }
    if (s.length() != t.length()) {
        return false;
    }
    for (int i = 0; i < s.length(); i++) {
        char c = s[i];
        char v = t[i];
        if (c != v) {
            if ('a' <= c && c <= 'z') {
                if (v != c - 'a' + 'A') {
                    return false;
                }
            } else if ('A' <= c && c <= 'Z') {
                if (v != c - 'A' + 'a') {
                    return false;
                }
            } else {
                return false;
            }
        }
    }
    return true;
}

// Map ...
inline string Map(const func<void(char&)>& mapping, const string& s) {
    string t(s);
    for (char& c : t) {
        mapping(c);
    }
    return t;
}

// ToLower ..
inline string ToLower(const string& s) {
    string t(s);
    for (char& c : t) {
        if ('A' <= c && c <= 'Z') {
            c = c - 'A' + 'a';
        }
    }
    return t;
}

// ToUpper ..
inline string ToUpper(const string& s) {
    string t(s);
    for (char& c : t) {
        if ('a' <= c && c <= 'z') {
            c = c - 'a' + 'A';
        }
    }
    return t;
}

// Cut ...
inline R<string /*before*/, string /*after*/, bool /*found*/> Cut(const string& s, const string& sep) {
    int i = Index(s, sep);
    if (i >= 0) {
        return {s.substr(0, i), s.substr(i + sep.length()), true};
    }
    return {s, "", false};
}

}  // namespace strings
}  // namespace gx

namespace gx {
namespace unitest {
void test_strings();
}  // namespace unitest
}  // namespace gx
