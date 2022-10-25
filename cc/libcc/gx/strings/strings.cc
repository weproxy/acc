//
// weproxy@foxmail.com 2022/10/03
//

#include "strings.h"

#include "gx/unicode/unicode.h"
#include "gx/unicode/utf8/utf8.h"

namespace gx {
namespace strings {

namespace xx {
// Generic split: splits after each instance of sep,
// including sepSave bytes of sep in the subarrays.
Vec<string> genSplit(const string& s, const string& sep, int sepSave, int n) {
    if (n == 0) {
        return {};
    }
    if (sep == "") {
        // return explode(s, n)
        return {s};
    }
    if (n < 0) {
        n = Count(s, sep) + 1;
    }

    if (n > s.length() + 1) {
        n = s.length() + 1;
    }

    Vec<string> a(n);
    // a.reserve(n);
    n--;
    int i = 0;
    string t = s;
    while (i < n) {
        int m = Index(s, sep);
        if (m < 0) {
            break;
        }
        a[i] = t.substr(0, m + sepSave);
        t = t.substr(m + sep.length());
        i++;
    }
    a[i] = t;

    return Vec<string>(a.begin(), a.begin() + i + 1);
}
}  // namespace xx

// IndexByte returns the index of the first instance of c in s, or -1 if c is not present in s.
int IndexByte(const string& s, byte c) {
    for (int i = 0; i < len(s); i++) {
        if (s[i] == c) {
            return i;
        }
    }
    return -1;
}

// Index returns the index of the first instance of substr in s, or -1 if substr is not present in s.
int Index(const string& s, const string& substr) {
    int sl = len(s), rl = len(substr);

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

// LastIndexByte returns the index of the last instance of c in s, or -1 if c is not present in s.
int LastIndexByte(const string& s, byte c) {
    for (int i = len(s) - 1; i >= 0; i--) {
        if (s[i] == c) {
            return i;
        }
    }
    return -1;
}

// LastIndex returns the index of the last instance of substr in s, or -1 if substr is not present in s.
int LastIndex(const string& s, const string& substr) {
    int sl = len(s), rl = len(substr);

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

// Count counts the number of non-overlapping instances of substr in s.
// If substr is an empty string, Count returns 1 + the number of Unicode code points in s.
int Count(const string& s, const string& substr) {
    int cnt = 0;
    int sl = len(s), rl = len(substr);
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

// Join ...
template <typename... T>
string Join(const string& sep, T&&... s) {
    return "";
}

// Repeat returns a new string consisting of count copies of the string s.
//
// It panics if count is negative or if
// the result of (len(s) * count) overflows.
string Repeat(const string& s, int count) {
    int nn = len(s) * count;
    char* buf = (char*)malloc(nn);
    for (int i = 0; i < count; i++) {
        memcpy(buf + i, s.c_str(), len(s));
    }
    string t(buf, nn);
    delete[] buf;
    return t;
}

namespace xx {
bool cutsetMatch(const string& cutset, char v) {
    for (char c : cutset) {
        if (c == v) {
            return true;
        }
    }
    return false;
}
int cutsetLeftIndex(const string& s, const string& cutset) {
    int i = 0;
    for (; i < len(s); i++) {
        if (!cutsetMatch(cutset, s[i])) {
            break;
        }
    }
    return i;
}
int cutsetRightIndex(const string& s, const string& cutset) {
    int i = len(s) - 1;
    for (; i >= 0; i--) {
        if (!cutsetMatch(cutset, s[i])) {
            break;
        }
    }
    return i;
}

// indexFunc ...
int indexFunc(const string& s, const func<bool(rune)>& f, bool truth) {
    for (int i = 0; i < len(s); i++) {
        if (f(s[i]) == truth) {
            return i;
        }
    }
    return -1;
}

// lastIndexFunc ...
int lastIndexFunc(const string& s, const func<bool(rune)>& f, bool truth) {
    for (int i = len(s); i > 0;) {
        AUTO_R(r, size, utf8::DecodeLastRuneInString(s.substr(0, i)));
        i -= size;
        if (f(s[i]) == truth) {
            return i;
        }
    }
    return -1;
}
}  // namespace xx

// TrimLeftFunc returns a slice of the string s with all leading
// Unicode code points c satisfying f(c) removed.
string TrimLeftFunc(const string& s, const func<bool(rune)>& f) {
    int i = xx::indexFunc(s, f, false);
    if (i == -1) {
        return "";
    }
    return s.substr(i);
}

// TrimRightFunc returns a slice of the string s with all trailing
// Unicode code points c satisfying f(c) removed.
string TrimRightFunc(const string& s, const func<bool(rune)>& f) {
    int i = xx::lastIndexFunc(s, f, false);
    if (i >= 0 && byte(s[i]) >= utf8::RuneSelf) {
        AUTO_R(_, wid, utf8::DecodeRuneInString(s.substr(i)));
        i += wid;
    } else {
        i++;
    }
    return s.substr(0, i);
}

// TrimLeft returns a slice of the string s with all leading
// Unicode code points contained in cutset removed.
//
// To remove a prefix, use TrimPrefix instead.
string TrimLeft(const string& s, const string& cutset) {
    if (s.empty() || cutset.empty()) {
        return s;
    }
    return string(s.c_str() + xx::cutsetLeftIndex(s, cutset));
}

// Trim returns a slice of the string s with all leading and
// trailing Unicode code points contained in cutset removed.
string Trim(const string& s, const string& cutset) {
    if (s.empty() || cutset.empty()) {
        return s;
    }
    int i = xx::cutsetLeftIndex(s, cutset);
    int j = xx::cutsetRightIndex(s, cutset);
    return string(s.c_str() + i, j);
}

// TrimRight returns a slice of the string s, with all trailing
// Unicode code points contained in cutset removed.
//
// To remove a suffix, use TrimSuffix instead.
string TrimRight(const string& s, const string& cutset) {
    if (s.empty() || cutset.empty()) {
        return s;
    }
    int i = xx::cutsetRightIndex(s, cutset);
    return i >= 0 ? string(s.c_str(), i + 1) : "";
}

// HasPrefix tests whether the string s begins with prefix.
bool HasPrefix(const string& s, const string& prefix) {
    return len(s) >= len(prefix) && memcmp(s.c_str(), prefix.c_str(), len(prefix)) == 0;
}

// HasSuffix tests whether the string s ends with suffix.
bool HasSuffix(const string& s, const string& suffix) {
    return len(s) >= len(suffix) && memcmp(s.c_str() + len(s) - len(suffix), suffix.c_str(), len(suffix)) == 0;
}

// TrimPrefix returns s without the provided leading prefix string.
// If s doesn't start with prefix, s is returned unchanged.
string TrimPrefix(const string& s, const string& prefix) {
    if (HasPrefix(s, prefix)) {
        return s.substr(len(prefix));
    }
    return s;
}

// TrimSuffix returns s without the provided trailing suffix string.
// If s doesn't end with suffix, s is returned unchanged.
string TrimSuffix(const string& s, const string& suffix) {
    if (HasSuffix(s, suffix)) {
        return s.substr(0, len(s) - len(suffix));
    }
    return s;
}

// Replace returns a copy of the string s with the first n
// non-overlapping instances of old replaced by new.
// If old is empty, it matches at the beginning of the string
// and after each UTF-8 sequence, yielding up to k+1 replacements
// for a k-rune string.
// If n < 0, there is no limit on the number of replacements.
string Replace(const string& s, const string& olds, const string& news, int n);

// EqualFold reports whether s and t, interpreted as UTF-8 strings,
// are equal under simple Unicode case-folding, which is a more general
// form of case-insensitivity.
bool EqualFold(const string& s, const string& t) {
    if (&s == &t) {
        return true;
    }
    if (len(s) != len(t)) {
        return false;
    }
    for (int i = 0; i < len(s); i++) {
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

// Map returns a copy of the string s with all its characters modified
// according to the mapping function. If mapping returns a negative value, the character is
// dropped from the string with no replacement.
string Map(const func<void(char&)>& mapping, const string& s) {
    string t(s);
    for (char& c : t) {
        mapping(c);
    }
    return t;
}

// ToLower returns s with all Unicode letters mapped to their lower case.
string ToLower(const string& s) {
    string t(s);
    for (char& c : t) {
        if ('A' <= c && c <= 'Z') {
            c = c - 'A' + 'a';
        }
    }
    return t;
}

// ToUpper returns s with all Unicode letters mapped to their upper case.
string ToUpper(const string& s) {
    string t(s);
    for (char& c : t) {
        if ('a' <= c && c <= 'z') {
            c = c - 'a' + 'A';
        }
    }
    return t;
}

// Cut slices s around the first instance of sep,
// returning the text before and after sep.
// The found result reports whether sep appears in s.
// If sep does not appear in s, cut returns s, "", false.
R<string /*before*/, string /*after*/, bool /*found*/> Cut(const string& s, const string& sep) {
    int i = Index(s, sep);
    if (i >= 0) {
        return {s.substr(0, i), s.substr(i + len(sep)), true};
    }
    return {s, "", false};
}

// Replace ...
string Replace(const string& s, const string& olds, const string& news, int n) {
    if (n == 0 || olds == news) {
        return s;  // avoid allocation
    }

    // Compute number of replacements.
    int m = Count(s, olds);
    if (m == 0) {
        return s;  // avoid allocation
    } else if (n < 0 || m < n) {
        n = m;
    }

    // Apply replacements to buffer.
    Builder b;
    b.Grow(len(s) + n * (len(news) - len(olds)));
    int start = 0;
    for (int i = 0; i < n; i++) {
        int j = start;
        if (len(olds) == 0) {
            if (i > 0) {
                AUTO_R(_, wid, utf8::DecodeRuneInString(s.substr(start)));
                j += wid;
            }
        } else {
            j += Index(s.substr(start), olds);
        }
        b.WriteString(s.substr(start, j));
        b.WriteString(news);
        start = j + len(olds);
    }
    b.WriteString(s.substr(start));
    return b.String();
}

// asciiSpace ...
static uint8 asciiSpace[256];
static auto _init_asciiSpace = [] {
    memset(asciiSpace, 0, sizeof(asciiSpace));
    const byte s[] = "\t\n\v\f\r ";
    for (int i = 0; i < sizeof(s); i++) {
        asciiSpace[s[i]] = 1;
    }
    return true;
}();

// Fields splits the string s around each instance of one or more consecutive white space
// characters, as defined by unicode.IsSpace, returning a slice of substrings of s or an
// empty slice if s contains only white space.
stringz<> Fields(const string& s) {
    // First count the fields.
    // This is an exact count if s is ASCII, otherwise it is an approximation.
    int n = 0;
    int wasSpace = 1;
    // setBits is used to track which bits are set in the bytes of s.
    uint8 setBits = 0;
    for (int i = 0; i < len(s); i++) {
        byte r = s[i];
        setBits |= r;
        int isSpace = int(asciiSpace[r]);
        n += wasSpace & ~isSpace;
        wasSpace = isSpace;
    }

    if (setBits >= utf8::RuneSelf) {
        // Some runes in the input string are not ASCII.
        return {FieldsFunc(s, unicode::IsSpace)};
    }
    // ASCII fast path
    stringz<> a = make<string>(n);
    int na = 0;
    int fieldStart = 0;
    int i = 0;
    // Skip spaces in the front of the input.
    while (i < len(s) && asciiSpace[s[i]] != 0) {
        i++;
    }
    fieldStart = i;
    while (i < len(s)) {
        if (asciiSpace[s[i]] == 0) {
            i++;
            continue;
        }
        a[na] = s.substr(fieldStart, i);
        na++;
        i++;
        // Skip spaces in between fields.
        while (i < len(s) && asciiSpace[s[i]] != 0) {
            i++;
        }
        fieldStart = i;
    }
    if (fieldStart < len(s)) {  // Last field might end at EOF.
        a[na] = s.substr(fieldStart);
    }
    return a;
}

// FieldsFunc splits the string s at each run of Unicode code points c satisfying f(c)
// and returns an array of slices of s. If all code points in s satisfy f(c) or the
// string is empty, an empty slice is returned.
//
// FieldsFunc makes no guarantees about the order in which it calls f(c)
// and assumes that f always returns the same value for a given c.
stringz<> FieldsFunc(const string& s, const func<bool(rune)>& f) {
    // A span is used to record a slice of s of the form s[start:end].
    // The start index is inclusive and the end index is exclusive.
    struct span {
        int start{0};
        int end{0};
        span() = default;
        span(int s, int e) : start(s), end(e) {}
    };
    auto spans = make<span>(0, 32);

    // Find the field start and end indices.
    // Doing this in a separate pass (rather than slicing the string s
    // and collecting the result substrings right away) is significantly
    // more efficient, possibly due to cache effects.
    int start = -1;  // valid span start if >= 0
    for (int end = 0; end < len(s); end++) {
        rune r = s[end];
        if (f(r)) {
            if (start >= 0) {
                spans = append(spans, span(start, end));
                // Set start to a negative value.
                // Note: using -1 here consistently and reproducibly
                // slows down this code by a several percent on amd64.
                start = ~start;
            }
        } else {
            if (start < 0) {
                start = end;
            }
        }
    }

    // Last field might end at EOF.
    if (start >= 0) {
        spans = append(spans, span{start, len(s)});
    }

    // Create strings from recorded field indices.
    auto a = make<string>(len(spans));
    // for i, span := range spans {
    for (int i = 0; i < len(spans); i++) {
        auto& span = spans[i];
        a[i] = s.substr(span.start, span.end);
    }

    return a;
}

// Join concatenates the elements of its first argument to create a single string. The separator
// string sep is placed between elements in the resulting string.
string Join(stringz<> elems, const string& sep) {
    if (len(elems) == 0) {
        return "";
    } else if (len(elems) == 1) {
        return elems[0];
    }

    int n = len(sep) * (len(elems) - 1);
    for (int i = 0; i < len(elems); i++) {
        n += len(elems[i]);
    }

    Builder b;
    b.Grow(n);
    b.WriteString(elems[0]);
    for (int i = 1; i < len(elems); i++) {
        b.WriteString(sep);
        b.WriteString(elems[i]);
    }
    return b.String();
}

}  // namespace strings
}  // namespace gx
