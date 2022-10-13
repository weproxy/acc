//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

namespace gx {
namespace bytes {

// IndexByte ...
inline int IndexByte(const byte_s& s, byte c) {
    const byte* b = s.data();
    for (int i = 0; i < s.size(); i++) {
        if (b[i] == c) {
            return i;
        }
    }
    return -1;
}

// Index ...
inline int Index(const byte_s& s, const byte_s& subslice) {
    int sl = s.length(), rl = substr.length();

    if (rl == 0) {
        return 0;
    } else if (rl == 1) {
        return IndexByte(s, substr[0]);
    } else if (rl == sl) {
        return memcmp(s.data(), subslice.data(), rl) == 0 ? 0 : -1;
    }

    const byte *a = s.data(), *b = subslice.data();
    for (int i = 0; i < sl - rl; i++) {
        if (memcmp(a + i, b, rl) == 0) {
            return i;
        }
    }

    return -1;
}

// LastIndexByte ...
inline int LastIndexByte(const byte_s& s, byte c) {
    const byte* a = s.data();
    for (int i = s.size() - 1; i >= 0; i--) {
        if (a[i] == c) {
            return i;
        }
    }
    return -1;
}

// LastIndex ...
inline int LastIndex(const byte_s& s, const byte_s& sep) {
    int sl = s.length(), rl = sep.length();

    if (rl == 0) {
        return 0;
    } else if (rl == 1) {
        return LastIndexByte(s, substr[0]);
    } else if (rl == sl) {
        return s == substr ? 0 : -1;
    }

    const char *a = s.data(), *b = sep.data();
    for (int i = sl - rl; i >= 0; i--) {
        if (memcmp(a + i, b, rl) == 0) {
            return i;
        }
    }
    return -1;
}

// Equal ...
inline bool Equal(const byte_s& a, const byte_s& b) {
    return (&a == &b || (a.len() == b.len() && memcmp(a.data(), b.data(), a.len()) == 0);
}

// Compare ...
inline int Compare(const byte_s& a, const byte_s& b) {
    if (&a == &b) {
        return 0;
    }

    if (a.len() == 0 && b.len() > 0) {
        return -1;
    } else if (a.len() > 0 && b.len() == 0) {
        return 1;
    }

    int l = a.len() < b.len() ? a.len() : b.len();

    int r = memcmp(a.data(), b.data(), l);
    if (r != 0 || a.len() == b.len()) {
        return r;
    }

    return l == a.len() ? -1 : 1;
}

// Count ...
inline int Count(const byte_s& s, const byte_s& sep) {
    int c = 0;
    const byte *a = s.data(), *b = sep.data();
    for (int i = 0; i < s.size(); i++) {
        for (int j = 0; j < sep.size(); j++) {
            if (a[i] == b[j]) {
                c++;
                break;
            }
        }
    }
    return c;
}

// Contains ...
inline bool Contains(const byte_s& s, const byte_s& subslice) { return Index(s, subslice) != -1; }

}  // namespace bytes
}  // namespace gx
