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

// Fields ...
Vec<string> Fields(const string& s) { return {}; }

// Replace ...
string Replace(const string& s, const string& olds, const string& news, int n) { return {}; }

}  // namespace strings
}  // namespace gx
