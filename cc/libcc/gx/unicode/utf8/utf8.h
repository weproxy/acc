//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"

namespace gx {
namespace utf8 {

// const ...
const int RuneError = 0xFFFD;    // the "error" Rune or "Unicode replacement character"
const int RuneSelf = 0x80;       // characters below RuneSelf are represented as themselves in a single byte.
const int MaxRune = 0x0010FFFF;  // Maximum valid Unicode code point.
const int UTFMax = 4;            // maximum number of bytes of a UTF-8 encoded Unicode character.

// RuneLen ...
int RuneLen(rune r);

// EncodeRune ..
int EncodeRune(slice<byte> p, rune r);

// EncodeRune ..
slice<byte> AppendRune(const slice<byte> p, rune r);

// RuneCount ...
int RuneCount(const slice<byte> p);

// RuneCountInString ...
int RuneCountInString(const string& s);

// RuneStart ...
inline bool RuneStart(byte b) { return (b & 0xC0) != 0x80; }

// Valid ...
bool Valid(const slice<byte> p);

// ValidString ...
bool ValidString(const string& s);

// ValidString ...
bool ValidRune(rune r);

// FullRune ...
bool FullRune(const slice<byte> p);

// FullRuneInString ...
bool FullRuneInString(const string& s);

// DecodeRune ...
R<rune, int> DecodeRune(const slice<byte> p);

// DecodeRuneInString ...
R<rune, int> DecodeRuneInString(const string& s);

// DecodeLastRune ...
R<rune, int> DecodeLastRune(const slice<byte> p);

// DecodeLastRuneInString ...
R<rune, int> DecodeLastRuneInString(const string& s);

}  // namespace utf8
}  // namespace gx
