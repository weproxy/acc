//
// weproxy@foxmail.com 2022/10/03
//

#include "utf8.h"

namespace gx {
namespace utf8 {

namespace xx {
constexpr int surrogateMin = 0xD800;
constexpr int surrogateMax = 0xDFFF;

constexpr uint8 t1 = 0b00000000;
constexpr uint8 tx = 0b10000000;
constexpr uint8 t2 = 0b11000000;
constexpr uint8 t3 = 0b11100000;
constexpr uint8 t4 = 0b11110000;
constexpr uint8 t5 = 0b11111000;

constexpr uint8 maskx = 0b00111111;
constexpr uint8 mask2 = 0b00011111;
constexpr uint8 mask3 = 0b00001111;
constexpr uint8 mask4 = 0b00000111;

constexpr int rune1Max = (1 << 7) - 1;
constexpr int rune2Max = (1 << 11) - 1;
constexpr int rune3Max = (1 << 16) - 1;

// The default lowest and highest continuation uint8.
constexpr uint8 locb = 0b10000000;
constexpr uint8 hicb = 0b10111111;

// These names of these constants are chosen to give nice alignment in the
// table below. The first nibble is an index into acceptRanges or F for
// special one-byte cases. The second nibble is the Rune length or the
// Status for the special one-byte case.
constexpr uint8 xx = 0xF1;  // invalid: size 1
constexpr uint8 as = 0xF0;  // ASCII: size 1
constexpr uint8 s1 = 0x02;  // accept 0, size 2
constexpr uint8 s2 = 0x13;  // accept 1, size 3
constexpr uint8 s3 = 0x03;  // accept 0, size 3
constexpr uint8 s4 = 0x23;  // accept 2, size 3
constexpr uint8 s5 = 0x34;  // accept 3, size 4
constexpr uint8 s6 = 0x04;  // accept 0, size 4
constexpr uint8 s7 = 0x44;  // accept 4, size 4

// first is information about the first byte in a UTF-8 sequence.
static constexpr uint8 first[256] = {
    //   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as,  // 0x00-0x0F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as,  // 0x10-0x1F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as,  // 0x20-0x2F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as,  // 0x30-0x3F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as,  // 0x40-0x4F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as,  // 0x50-0x5F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as,  // 0x60-0x6F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as,  // 0x70-0x7F
    //   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,  // 0x80-0x8F
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,  // 0x90-0x9F
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,  // 0xA0-0xAF
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,  // 0xB0-0xBF
    xx, xx, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1,  // 0xC0-0xCF
    s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1,  // 0xD0-0xDF
    s2, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s4, s3, s3,  // 0xE0-0xEF
    s5, s6, s6, s6, s7, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,  // 0xF0-0xFF
};

// acceptRange gives the range of valid values for the second byte in a UTF-8
// sequence.
struct acceptRange {
    uint8 lo;  // lowest value for second byte.
    uint8 hi;  // highest value for second byte.
};

// acceptRanges has size 16 to avoid bounds checks in the code that uses it.
static acceptRange acceptRanges[16] = {
    {locb, hicb}, {0xA0, hicb}, {locb, 0x9F}, {0x90, hicb}, {locb, 0x8F},
};

}  // namespace xx

// RuneLen ...
int RuneLen(rune r) {
    if (r < 0) {
        return -1;
    } else if (r <= xx::rune1Max) {
        return 1;
    } else if (r <= xx::rune2Max) {
        return 2;
    } else if (xx::surrogateMin <= r && r <= xx::surrogateMax) {
        return -1;
    } else if (r <= xx::rune3Max) {
        return 3;
    } else if (r <= MaxRune) {
        return 4;
    }
    return -1;
}

// EncodeRune ..
int EncodeRune(bytez<> p, rune r) {
    // Negative values are erroneous. Making it unsigned addresses the problem.
    uint32 i = uint32(r);
    if (i <= xx::rune1Max) {
        p[0] = byte(r);
        return 1;
    } else if (i <= xx::rune2Max) {
        p[0] = xx::t2 | byte(r >> 6);
        p[1] = xx::tx | byte(r) & xx::maskx;
        return 2;
    } else {
        if (i > MaxRune || (xx::surrogateMin <= i && i <= xx::surrogateMax)) {
            r = RuneError;
        }
        if (i <= xx::rune3Max) {
            p[0] = xx::t3 | byte(r >> 12);
            p[1] = xx::tx | byte(r >> 6) & xx::maskx;
            p[2] = xx::tx | byte(r) & xx::maskx;
            return 3;
        } else {
            p[0] = xx::t4 | byte(r >> 18);
            p[1] = xx::tx | byte(r >> 12) & xx::maskx;
            p[2] = xx::tx | byte(r >> 6) & xx::maskx;
            p[3] = xx::tx | byte(r) & xx::maskx;
            return 4;
        }
    }
}

// appendRuneNonASCII ...
static bytez<> appendRuneNonASCII(const bytez<> p, rune r) {
    // Negative values are erroneous. Making it unsigned addresses the problem.
    uint32 i = uint32(r);
    if (i <= xx::rune2Max) {
        return append(p, xx::t2 | byte(r >> 6), xx::tx | byte(r) & xx::maskx);
    } else {
        if (i > MaxRune || (xx::surrogateMin <= i && i <= xx::surrogateMax)) {
            r = RuneError;
        }

        if (i <= xx::rune3Max) {
            return append(p, xx::t3 | byte(r >> 12), xx::tx | byte(r >> 6) & xx::maskx, xx::tx | byte(r) & xx::maskx);
        } else {
            return append(p, xx::t4 | byte(r >> 18), xx::tx | byte(r >> 12) & xx::maskx,
                          xx::tx | byte(r >> 6) & xx::maskx, xx::tx | byte(r) & xx::maskx);
        }
    }
}

// EncodeRune ..
bytez<> AppendRune(const bytez<>& p, rune r) {
    // This function is inlineable for fast handling of ASCII.
    if (uint32(r) <= xx::rune1Max) {
        return append(p, byte(r));
    }
    return appendRuneNonASCII(p, r);
}

// RuneCount ...
int RuneCount(const bytez<>& p) {
    int n = 0;
    int np = len(p);
    for (int i = 0; i < np;) {
        n++;
        byte c = p[i];
        if (c < RuneSelf) {
            // ASCII fast path
            i++;
            continue;
        }
        byte x = xx::first[c];
        if (x == xx::xx) {
            i++;  // invalid.
            continue;
        }
        int size = int(x & 7);
        if (i + size > np) {
            i++;  // Short or invalid.
            continue;
        }
        const auto& accept = xx::acceptRanges[x >> 4];
        if (p[i + 1] < accept.lo || accept.hi < p[i + 1]) {
            size = 1;
        } else if (size == 2) {
        } else if (p[i + 2] < xx::locb || xx::hicb < p[i + 2]) {
            size = 1;
        } else if (size == 3) {
        } else if (p[i + 3] < xx::locb || xx::hicb < p[i + 3]) {
            size = 1;
        }
        i += size;
    }
    return n;
}

// RuneCountInString ...
int RuneCountInString(const string& s) {
    int n = 0;
    int ns = len(s);
    for (int i = 0; i < ns; n++) {
        byte c = s[i];
        if (c < RuneSelf) {
            // ASCII fast path
            i++;
            continue;
        }
        byte x = xx::first[c];
        if (x == xx::xx) {
            i++;  // invalid.
            continue;
        }
        int size = int(x & 7);
        if (i + size > ns) {
            i++;  // Short or invalid.
            continue;
        }
        const auto& accept = xx::acceptRanges[x >> 4];
        if (byte(s[i + 1]) < accept.lo || accept.hi < byte(s[i + 1])) {
            size = 1;
        } else if (size == 2) {
        } else if (byte(s[i + 2]) < xx::locb || xx::hicb < byte(s[i + 2])) {
            size = 1;
        } else if (size == 3) {
        } else if (byte(s[i + 3]) < xx::locb || xx::hicb < byte(s[i + 3])) {
            size = 1;
        }
        i += size;
    }
    return n;
}

// Valid ...
bool Valid(const bytez<>& p1) {
    // This optimization avoids the need to recompute the capacity
    // when generating code for p[8:], bringing it to parity with
    // ValidString, which was 20% faster on long ASCII strings.
    bytez<> p = p1(0, len(p1));

    // Fast path. Check for and skip 8 bytes of ASCII characters per iteration.
    while (len(p) >= 8) {
        // Combining two 32 bit loads allows the same code to be used
        // for 32 and 64 bit platforms.
        // The compiler can generate a 32bit load for first32 and second32
        // on many platforms. See test/codegen/memcombine.go.
        uint32 first32 = uint32(p[0]) | uint32(p[1]) << 8 | uint32(p[2]) << 16 | uint32(p[3]) << 24;
        uint32 second32 = uint32(p[4]) | uint32(p[5]) << 8 | uint32(p[6]) << 16 | uint32(p[7]) << 24;
        if ((first32 | (second32) & 0x80808080) != 0) {
            // Found a non ASCII byte (>= RuneSelf).
            break;
        }
        p = p(8);
    }
    int n = len(p);
    for (int i = 0; i < n;) {
        byte pi = p[i];
        if (pi < RuneSelf) {
            i++;
            continue;
        }
        byte x = xx::first[pi];
        if (x == xx::xx) {
            return false;  // Illegal starter byte.
        }
        int size = int(x & 7);
        if (i + size > n) {
            return false;  // Short or invalid.
        }
        const auto& accept = xx::acceptRanges[x >> 4];
        if (p[i + 1] < accept.lo || accept.hi < p[i + 1]) {
            return false;
        } else if (size == 2) {
        } else if (p[i + 2] < xx::locb || xx::hicb < p[i + 2]) {
            return false;
        } else if (size == 3) {
        } else if (p[i + 3] < xx::locb || xx::hicb < p[i + 3]) {
            return false;
        }
        i += size;
    }
    return true;
}

// ValidString ...
bool ValidString(const string& s1) {
    string s = s1;
    // Fast path. Check for and skip 8 bytes of ASCII characters per iteration.
    while (len(s) >= 8) {
        // Combining two 32 bit loads allows the same code to be used
        // for 32 and 64 bit platforms.
        // The compiler can generate a 32bit load for first32 and second32
        // on many platforms. See test/codegen/memcombine.go.
        uint32 first32 = uint32(s[0]) | uint32(s[1]) << 8 | uint32(s[2]) << 16 | uint32(s[3]) << 24;
        uint32 second32 = uint32(s[4]) | uint32(s[5]) << 8 | uint32(s[6]) << 16 | uint32(s[7]) << 24;
        if ((first32 | (second32) & 0x80808080) != 0) {
            // Found a non ASCII byte (>= RuneSelf).
            break;
        }
        s = s.substr(8);
    }
    int n = len(s);
    for (int i = 0; i < n;) {
        byte si = s[i];
        if (si < RuneSelf) {
            i++;
            continue;
        }
        byte x = xx::first[si];
        if (x == xx::xx) {
            return false;  // Illegal starter byte.
        }
        int size = int(x & 7);
        if (i + size > n) {
            return false;  // Short or invalid.
        }
        const auto& accept = xx::acceptRanges[x >> 4];
        if (byte(s[i + 1]) < accept.lo || accept.hi < byte(s[i + 1])) {
            return false;
        } else if (size == 2) {
        } else if (byte(s[i + 2]) < xx::locb || xx::hicb < byte(s[i + 2])) {
            return false;
        } else if (size == 3) {
        } else if (byte(s[i + 3]) < xx::locb || xx::hicb < byte(s[i + 3])) {
            return false;
        }
        i += size;
    }
    return true;
}

// ValidString ...
bool ValidRune(rune r) {
    if (0 <= r && r < xx::surrogateMin) {
        return true;
    } else if (xx::surrogateMax < r && r <= MaxRune) {
        return true;
    }
    return false;
}

// FullRune reports whether the bytes in p begin with a full UTF-8 encoding of a rune.
// An invalid encoding is considered a full Rune since it will convert as a width-1 error rune.
bool FullRune(const bytez<>& p) {
    int n = len(p);
    if (n == 0) {
        return false;
    }
    byte x = xx::first[p[0]];
    if (n >= int(x & 7)) {
        return true;  // ASCII, invalid or valid.
    }
    // Must be short or invalid.
    const auto& accept = xx::acceptRanges[x >> 4];
    if (n > 1 && (p[1] < accept.lo || accept.hi < p[1])) {
        return true;
    } else if (n > 2 && (p[2] < xx::locb || xx::hicb < p[2])) {
        return true;
    }
    return false;
}

// FullRuneInString is like FullRune but its input is a string.
bool FullRuneInString(const string& s) {
    int n = len(s);
    if (n == 0) {
        return false;
    }
    byte x = xx::first[s[0]];
    if (n >= int(x & 7)) {
        return true;  // ASCII, invalid, or valid.
    }
    // Must be short or invalid.
    const auto& accept = xx::acceptRanges[x >> 4];
    if (n > 1 && (byte(s[1]) < accept.lo || accept.hi < byte(s[1]))) {
        return true;
    } else if (n > 2 && (byte(s[2]) < xx::locb || xx::hicb < byte(s[2]))) {
        return true;
    }
    return false;
}

// DecodeRune ...
R<rune, int> DecodeRune(const bytez<>& p) {
    int n = len(p);
    if (n < 1) {
        return {RuneError, 0};
    }
    byte p0 = p[0];
    byte x = xx::first[p0];
    if (x >= xx::as) {
        // The following code simulates an additional check for x == xx and
        // handling the ASCII and invalid cases accordingly. This mask-and-or
        // approach prevents an additional branch.
        rune mask = rune(x) << 31 >> 31;  // Create 0x0000 or 0xFFFF.
        return {rune(p[0]) & ~mask | RuneError & mask, 1};
    }
    int sz = int(x & 7);
    const auto& accept = xx::acceptRanges[x >> 4];
    if (n < sz) {
        return {RuneError, 1};
    }
    byte b1 = p[1];
    if (b1 < accept.lo || accept.hi < b1) {
        return {RuneError, 1};
    }
    if (sz <= 2) {  // <= instead of == to help the compiler eliminate some bounds checks
        return {rune(p0 & xx::mask2) << 6 | rune(b1 & xx::maskx), 2};
    }
    byte b2 = p[2];
    if (b2 < xx::locb || xx::hicb < b2) {
        return {RuneError, 1};
    }
    if (sz <= 3) {
        return {rune(p0 & xx::mask3) << 12 | rune(b1 & xx::maskx) << 6 | rune(b2 & xx::maskx), 3};
    }
    byte b3 = p[3];
    if (b3 < xx::locb || xx::hicb < b3) {
        return {RuneError, 1};
    }
    return {rune(p0 & xx::mask4) << 18 | rune(b1 & xx::maskx) << 12 | rune(b2 & xx::maskx) << 6 | rune(b3 & xx::maskx),
            4};
}

// DecodeRuneInString ...
R<rune, int> DecodeRuneInString(const string& s) {
    int n = len(s);
    if (n < 1) {
        return {RuneError, 0};
    }
    byte s0 = s[0];
    byte x = xx::first[s0];
    if (x >= xx::as) {
        // The following code simulates an additional check for x == xx and
        // handling the ASCII and invalid cases accordingly. This mask-and-or
        // approach prevents an additional branch.
        rune mask = rune(x) << 31 >> 31;  // Create 0x0000 or 0xFFFF.
        return {rune(s[0]) & ~mask | RuneError & mask, 1};
    }
    byte sz = int(x & 7);
    const auto& accept = xx::acceptRanges[x >> 4];
    if (n < sz) {
        return {RuneError, 1};
    }
    byte s1 = s[1];
    if (s1 < accept.lo || accept.hi < s1) {
        return {RuneError, 1};
    }
    if (sz <= 2) {  // <= instead of == to help the compiler eliminate some bounds checks
        return {rune(s0 & xx::mask2) << 6 | rune(s1 & xx::maskx), 2};
    }
    byte s2 = s[2];
    if (s2 < xx::locb || xx::hicb < s2) {
        return {RuneError, 1};
    }
    if (sz <= 3) {
        return {rune(s0 & xx::mask3) << 12 | rune(s1 & xx::maskx) << 6 | rune(s2 & xx::maskx), 3};
    }
    byte s3 = s[3];
    if (s3 < xx::locb || xx::hicb < s3) {
        return {RuneError, 1};
    }
    return {rune(s0 & xx::mask4) << 18 | rune(s1 & xx::maskx) << 12 | rune(s2 & xx::maskx) << 6 | rune(s3 & xx::maskx),
            4};
}

// DecodeLastRune ...
R<rune, int> DecodeLastRune(const bytez<>& p) {
    rune r;
    int size = 0;

    int end = len(p);
    if (end == 0) {
        return {RuneError, 0};
    }
    int start = end - 1;
    r = rune(p[start]);
    if (r < RuneSelf) {
        return {r, 1};
    }
    // guard against O(n^2) behavior when traversing
    // backwards through strings with long sequences of
    // invalid UTF-8.
    int lim = end - UTFMax;
    if (lim < 0) {
        lim = 0;
    }
    for (start--; start >= lim; start--) {
        if (RuneStart(p[start])) {
            break;
        }
    }
    if (start < 0) {
        start = 0;
    }
    AUTO_R(_r, _size, DecodeRune(p(start, end)));
    r = _r;
    size = _size;
    if (start + size != end) {
        return {RuneError, 1};
    }
    return {r, size};
}

// DecodeLastRuneInString ...
R<rune, int> DecodeLastRuneInString(const string& s) {
    rune r;
    int size = 0;

    int end = len(s);
    if (end == 0) {
        return {RuneError, 0};
    }
    int start = end - 1;
    r = rune(s[start]);
    if (r < RuneSelf) {
        return {r, 1};
    }
    // guard against O(n^2) behavior when traversing
    // backwards through strings with long sequences of
    // invalid UTF-8.
    int lim = end - UTFMax;
    if (lim < 0) {
        lim = 0;
    }
    for (start--; start >= lim; start--) {
        if (RuneStart(s[start])) {
            break;
        }
    }
    if (start < 0) {
        start = 0;
    }
    AUTO_R(_r, _size, DecodeRuneInString(s.substr(start, end)));
    r = _r;
    size = _size;
    if (start + size != end) {
        return {RuneError, 1};
    }
    return {r, size};
}

}  // namespace utf8
}  // namespace gx
