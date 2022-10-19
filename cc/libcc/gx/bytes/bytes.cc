//
// weproxy@foxmail.com 2022/10/03
//

#include "bytes.h"

#include "gx/errors/errors.h"
#include "gx/unicode/utf8/utf8.h"

namespace gx {
namespace bytes {
// ErrTooLarge is passed to panic if memory cannot be allocated to store data in a buffer.
error ErrTooLarge = errors::New("bytes.Buffer: too large");

// IndexByte returns the index of the first instance of c in b, or -1 if c is not present in b.
int IndexByte(const slice<byte>& s, byte c) {
    const byte* b = s.data();
    for (int i = 0; i < len(s); i++) {
        if (b[i] == c) {
            return i;
        }
    }
    return -1;
}

// Index returns the index of the first instance of sep in s, or -1 if sep is not present in s.
int Index(const slice<byte>& s, const slice<byte>& subslice) {
    int sl = s.length(), rl = subslice.length();

    if (rl == 0) {
        return 0;
    } else if (rl == 1) {
        return IndexByte(s, subslice[0]);
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
int LastIndexByte(const slice<byte>& s, byte c) {
    const byte* a = s.data();
    for (int i = len(s) - 1; i >= 0; i--) {
        if (a[i] == c) {
            return i;
        }
    }
    return -1;
}

// LastIndex returns the index of the last instance of sep in s, or -1 if sep is not present in s.
int LastIndex(const slice<byte>& s, const slice<byte>& sep) {
    int sl = s.length(), rl = sep.length();

    if (rl == 0) {
        return 0;
    } else if (rl == 1) {
        return LastIndexByte(s, sep[0]);
    } else if (rl == sl) {
        return memcmp(s.data(), sep.data(), rl) == 0 ? 0 : -1;
    }

    const byte *a = s.data(), *b = sep.data();
    for (int i = sl - rl; i >= 0; i--) {
        if (memcmp(a + i, b, rl) == 0) {
            return i;
        }
    }
    return -1;
}

// Equal reports whether a and b
// are the same length and contain the same bytes.
// A nil argument is equivalent to an empty slice.
bool Equal(const slice<byte>& a, const slice<byte>& b) {
    return (&a == &b || (a.len() == b.len() && memcmp(a.data(), b.data(), a.len()) == 0));
}

// Compare returns an integer comparing two byte slices lexicographically.
// The result will be 0 if a == b, -1 if a < b, and +1 if a > b.
// A nil argument is equivalent to an empty slice.
int Compare(const slice<byte>& a, const slice<byte>& b) {
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

// Count counts the number of non-overlapping instances of sep in s.
// If sep is an empty slice, Count returns 1 + the number of UTF-8-encoded code points in s.
int Count(const slice<byte>& s, const slice<byte>& sep) {
    int c = 0;
    const byte *a = s.data(), *b = sep.data();
    for (int i = 0; i < len(s); i++) {
        for (int j = 0; j < len(sep); j++) {
            if (a[i] == b[j]) {
                c++;
                break;
            }
        }
    }
    return c;
}

namespace bytealg {
const int MaxLen = 1024;
// IndexByteString ...
int IndexByteString(const string& chars, byte c) {
    for (int i = 0; i < len(chars); i++) {
        if (chars[i] == c) {
            return i;
        }
    }
    return -1;
}

// IndexByteString ...
int IndexString(const string& chars, const string& s) { return -1; }

}  // namespace bytealg

// IndexRune interprets s as a sequence of UTF-8-encoded code points.
// It returns the byte index of the first occurrence in s of the given rune.
// It returns -1 if rune is not present in s.
// If r is utf8.RuneError, it returns the first instance of any
// invalid UTF-8 byte sequence.
int IndexRune(const slice<byte>& s, rune r) {
    if (0 <= r && r < utf8::RuneSelf) {
        return IndexByte(s, byte(r));
    } else if (r == utf8::RuneError) {
        for (int i = 0; i < len(s);) {
            AUTO_R(r1, n, utf8::DecodeRune(s(i)));
            if (r1 == utf8::RuneError) {
                return i;
            }
            i += n;
        }
        return -1;
    } else if (!utf8::ValidRune(r)) {
        return -1;
    } else {
        auto b = make(utf8::UTFMax);
        int n = utf8::EncodeRune(b, r);
        return Index(s, b(0, n));
    }
}

// asciiSet is a 32-byte value, where each bit represents the presence of a
// given ASCII character in the set. The 128-bits of the lower 16 bytes,
// starting with the least-significant bit of the lowest word to the
// most-significant bit of the highest word, map to the full range of all
// 128 ASCII characters. The 128-bits of the upper 16 bytes will be zeroed,
// ensuring that any non-ASCII character will be reported as not in the set.
// This allocates a total of 32 bytes even though the upper half
// is unused to avoid bounds checks in asciiSet.contains.
struct asciiSet {
    uint32 as[8];

    // contains reports whether c is inside the set.
    bool contains(byte c) { return (as[c / 32] & (1 << (c % 32))) != 0; }
};

// makeASCIISet creates a set of ASCII characters and reports whether all
// characters in chars are ASCII.
R<asciiSet, bool> makeASCIISet(const string& chars) {
    asciiSet as;
    for (int i = 0; i < len(chars); i++) {
        byte c = chars[i];
        if (c >= utf8::RuneSelf) {
            return {as, false};
        }
        as.as[c / 32] |= 1 << (c % 32);
    }
    return {as, true};
}

// IndexAny interprets s as a sequence of UTF-8-encoded Unicode code points.
// It returns the byte index of the first occurrence in s of any of the Unicode
// code points in chars. It returns -1 if chars is empty or if there is no code
// point in common.
int IndexAny(const slice<byte>& s, const string& chars) {
    if (chars == "") {
        // Avoid scanning all of s.
        return -1;
    }
    if (len(s) == 1) {
        rune r = rune(s[0]);
        if (r >= utf8::RuneSelf) {
            // search utf8.RuneError.
            for (auto r : chars) {
                if (rune(r) == utf8::RuneError) {
                    return 0;
                }
            }
            return -1;
        }
        if (bytealg::IndexByteString(chars, s[0]) >= 0) {
            return 0;
        }
        return -1;
    }
    if (len(chars) == 1) {
        rune r = rune(chars[0]);
        if (r >= utf8::RuneSelf) {
            r = utf8::RuneError;
        }
        return IndexRune(s, r);
    }
    if (len(s) > 8) {
        AUTO_R(as, isASCII, makeASCIISet(chars));
        if (isASCII) {
            // for i, c := range s {
            for (int i = 0; i < len(s); i++) {
                auto c = s[i];
                if (as.contains(c)) {
                    return i;
                }
            }
            return -1;
        }
    }
    int width = 0;
    for (int i = 0; i < len(s); i += width) {
        rune r = rune(s[i]);
        if (r < utf8::RuneSelf) {
            if (bytealg::IndexByteString(chars, s[i]) >= 0) {
                return i;
            }
            width = 1;
            continue;
        }
        AUTO_R(_r, _width, utf8::DecodeRune(s(i)));
        r = _r;
        width = _width;
        if (r != utf8::RuneError) {
            // r is 2 to 4 bytes
            if (len(chars) == width) {
                if (chars == string((char*)&r, sizeof(r))) {
                    return i;
                }
                continue;
            }
            // Use bytealg.IndexString for performance if available.
            if (bytealg::MaxLen >= width) {
                if (bytealg::IndexString(chars, string((char*)&r, sizeof(r))) >= 0) {
                    return i;
                }
                continue;
            }
        }
        for (auto ch : chars) {
            if (r == ch) {
                return i;
            }
        }
    }
    return -1;
}

}  // namespace bytes
}  // namespace gx
