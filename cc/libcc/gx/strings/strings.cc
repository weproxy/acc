//
// weproxy@foxmail.com 2022/10/03
//

#include "strings.h"

namespace gx {
namespace strings {

namespace xx {
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

// // asciiSpace ...
// static uint8 asciiSpace[256]{'\t' : 1, '\n' : 1, '\v' : 1, '\f' : 1, '\r' : 1, ' ' : 1};

// // Fields ...
// slice<string> Fields(const string& s) {
//     // First count the fields.
//     // This is an exact count if s is ASCII, otherwise it is an approximation.
//     int n = 0;
//     int wasSpace = 1;
//     // setBits is used to track which bits are set in the bytes of s.
//     uint8 setBits = uint8(0);
//     for (int i = 0; i < len(s); i++) {
//         byte r = s[i];
//         setBits |= r;
//         int isSpace = int(asciiSpace[r]);
//         n += wasSpace & ~isSpace;
//         wasSpace = isSpace;
//     }

//     if (setBits >= utf8::RuneSelf) {
//         // Some runes in the input string are not ASCII.
//         return {FieldsFunc(s, unicode.IsSpace)};
//     }
//     // ASCII fast path
//     slice<string> a = make<string>(, n);
//     int na = 0;
//     int fieldStart = 0;
//     int i = 0;
//     // Skip spaces in the front of the input.
//     for (; i < len(s) && asciiSpace[s[i]] != 0;) {
//         i++;
//     }
//     fieldStart = i;
//     for (; i < len(s);) {
//         if (asciiSpace[s[i]] == 0) {
//             i++;
//             continue;
//         }
//         a[na] = s [fieldStart:i];
//         na++;
//         i++;
//         // Skip spaces in between fields.
//         for (; i < len(s) && asciiSpace[s[i]] != 0;) {
//             i++;
//         }
//         fieldStart = i;
//     }
//     if (fieldStart < len(s)) {  // Last field might end at EOF.
//         a[na] = s [fieldStart:];
//     }
//     return a;
// }

}  // namespace strings
}  // namespace gx
