//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"

namespace gx {
namespace utf8 {

// The conditions RuneError==unicode.ReplacementChar and
// MaxRune==unicode.MaxRune are verified in the tests.
// Defining them locally avoids this package depending on package unicode.

// Numbers fundamental to the encoding.
constexpr int RuneError = 0xFFFD;    // the "error" Rune or "Unicode replacement character"
constexpr int RuneSelf = 0x80;       // characters below RuneSelf are represented as themselves in a single byte.
constexpr int MaxRune = 0x0010FFFF;  // Maximum valid Unicode code point.
constexpr int UTFMax = 4;            // maximum number of bytes of a UTF-8 encoded Unicode character.

// RuneLen returns the number of bytes required to encode the rune.
// It returns -1 if the rune is not a valid value to encode in UTF-8.
int RuneLen(rune r);

// EncodeRune writes into p (which must be large enough) the UTF-8 encoding of the rune.
// If the rune is out of range, it writes the encoding of RuneError.
// It returns the number of bytes written.
int EncodeRune(bytez<> p, rune r);

// AppendRune appends the UTF-8 encoding of r to the end of p and
// returns the extended buffer. If the rune is out of range,
// it appends the encoding of RuneError.
bytez<> AppendRune(const bytez<>& p, rune r);

// RuneCount returns the number of runes in p. Erroneous and short
// encodings are treated as single runes of width 1 byte.
int RuneCount(const bytez<>& p);

// RuneCountInString is like RuneCount but its input is a string.
int RuneCountInString(const string& s);

// RuneStart reports whether the byte could be the first byte of an encoded,
// possibly invalid rune. Second and subsequent bytes always have the top two
// bits set to 10.
inline bool RuneStart(byte b) { return (b & 0xC0) != 0x80; }

// Valid reports whether p consists entirely of valid UTF-8-encoded runes.
bool Valid(const bytez<>& p);

// ValidString reports whether s consists entirely of valid UTF-8-encoded runes.
bool ValidString(const string& s);

// ValidRune reports whether r can be legally encoded as UTF-8.
// Code points that are out of range or a surrogate half are illegal.
bool ValidRune(rune r);

// FullRune reports whether the bytes in p begin with a full UTF-8 encoding of a rune.
// An invalid encoding is considered a full Rune since it will convert as a width-1 error rune.
bool FullRune(const bytez<> p);

// FullRuneInString is like FullRune but its input is a string.
bool FullRuneInString(const string& s);

// DecodeRune unpacks the first UTF-8 encoding in p and returns the rune and
// its width in bytes. If p is empty it returns (RuneError, 0). Otherwise, if
// the encoding is invalid, it returns (RuneError, 1). Both are impossible
// results for correct, non-empty UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
R<rune, int> DecodeRune(const bytez<>& p);

// DecodeRuneInString is like DecodeRune but its input is a string. If s is
// empty it returns (RuneError, 0). Otherwise, if the encoding is invalid, it
// returns (RuneError, 1). Both are impossible results for correct, non-empty
// UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
R<rune, int> DecodeRuneInString(const string& s);

// DecodeLastRune unpacks the last UTF-8 encoding in p and returns the rune and
// its width in bytes. If p is empty it returns (RuneError, 0). Otherwise, if
// the encoding is invalid, it returns (RuneError, 1). Both are impossible
// results for correct, non-empty UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
R<rune, int> DecodeLastRune(const bytez<>& p);

// DecodeLastRuneInString is like DecodeLastRune but its input is a string. If
// s is empty it returns (RuneError, 0). Otherwise, if the encoding is invalid,
// it returns (RuneError, 1). Both are impossible results for correct,
// non-empty UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
R<rune, int> DecodeLastRuneInString(const string& s);

}  // namespace utf8
}  // namespace gx
