//
// weproxy@foxmail.com 2022/10/03
//

#include "hex.h"

#include "gx/errors/errors.h"
#include "gx/fmt/fmt.h"

namespace gx {
namespace hex {

namespace xx {

static const char* hextable = "0123456789abcdef";
static const char* reverseHexTable =
    ""
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\xff\xff\xff\xff\xff\xff"
    "\xff\x0a\x0b\x0c\x0d\x0e\x0f\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\x0a\x0b\x0c\x0d\x0e\x0f\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

// ErrLength ...
static const error ErrLength = errors::New("encoding/hex: odd length hex string");

// InvalidByteError ...
static error InvalidByteError(byte e) { return fmt::Errorf("encoding/hex: invalid byte: %d", e); }

static int EncodedLen(int n) { return n * 2; }
static int DecodedLen(int x) { return x / 2; }

static int Encode(byte dst[], int dstLen, const byte src[], int srcLen) {
    int j = 0;
    for (int i = 0; i < srcLen; i++) {
        byte v = src[i];
        dst[j] = hextable[v >> 4];
        dst[j + 1] = hextable[v & 0x0f];
        j += 2;
    }
    return srcLen * 2;
}

static R<int, error> Decode(byte dst[], int dstLen, const byte src[], int srcLen) {
    int i = 0, j = 1;
    for (; j < srcLen; j += 2) {
        byte p = src[j - 1];
        byte q = src[j];

        byte a = reverseHexTable[p];
        byte b = reverseHexTable[q];
        if (a > 0x0f) {
            return {i, InvalidByteError(p)};
        }
        if (b > 0x0f) {
            return {i, InvalidByteError(q)};
        }
        dst[i] = (a << 4) | b;
        i++;
    }
    if (srcLen % 2 == 1) {
        // Check for invalid char before reporting bad length,
        // since the invalid char (if present) is an earlier problem.
        if (reverseHexTable[src[j - 1]] > 0x0f) {
            return {i, InvalidByteError(src[j - 1])};
        }
        return {i, ErrLength};
    }
    return {i, nil};
}

}  // namespace xx

// EncodeToString ...
string EncodeToString(const void* src, size_t srcLen) {
    int dstLen = xx::EncodedLen(srcLen);
    byte* dst = (uint8*)malloc(dstLen);
    xx::Encode(dst, dstLen, (const byte*)src, srcLen);
    string s((char*)dst, dstLen);
    free(dst);
    return s;
}

// DecodeString ...
R<slice<byte>, error> DecodeString(const string& s) {
    const byte* src = (const byte*)s.data();
    int srcLen = s.size();

    slice<byte> r(0, srcLen);
    AUTO_R(dstLen, err, xx::Decode(r.data(), srcLen, src, srcLen));
    if (err) {
        return {{}, err};
    }
    r.end_ = dstLen;

    return {r, nil};
}

}  // namespace hex
}  // namespace gx
