//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"

namespace gx {
namespace bytes {

// IndexByte returns the index of the first instance of c in b, or -1 if c is not present in b.
int IndexByte(const slice<byte>& s, byte c);

// Index returns the index of the first instance of sep in s, or -1 if sep is not present in s.
int Index(const slice<byte>& s, const slice<byte>& subslice);

// LastIndexByte ...
int LastIndexByte(const slice<byte>& s, byte c);

// LastIndex returns the index of the last instance of sep in s, or -1 if sep is not present in s.
int LastIndex(const slice<byte>& s, const slice<byte>& sep);

// Equal reports whether a and b
// are the same length and contain the same bytes.
// A nil argument is equivalent to an empty slice.
bool Equal(const slice<byte>& a, const slice<byte>& b);

// Compare returns an integer comparing two byte slices lexicographically.
// The result will be 0 if a == b, -1 if a < b, and +1 if a > b.
// A nil argument is equivalent to an empty slice.
int Compare(const slice<byte>& a, const slice<byte>& b);

// Count counts the number of non-overlapping instances of sep in s.
// If sep is an empty slice, Count returns 1 + the number of UTF-8-encoded code points in s.
int Count(const slice<byte>& s, const slice<byte>& sep);

// Contains reports whether subslice is within b.
inline bool Contains(const slice<byte>& s, const slice<byte>& subslice) { return Index(s, subslice) != -1; }

// IndexRune interprets s as a sequence of UTF-8-encoded code points.
// It returns the byte index of the first occurrence in s of the given rune.
// It returns -1 if rune is not present in s.
// If r is utf8.RuneError, it returns the first instance of any
// invalid UTF-8 byte sequence.
int IndexRune(const slice<byte>& s, rune r);

// IndexAny interprets s as a sequence of UTF-8-encoded Unicode code points.
// It returns the byte index of the first occurrence in s of any of the Unicode
// code points in chars. It returns -1 if chars is empty or if there is no code
// point in common.
int IndexAny(const slice<byte>& s, const string& chars);

// ContainsAny reports whether any of the UTF-8-encoded code points in chars are within b.
inline bool ContainsAny(const slice<byte>& b, const string& chars) { return IndexAny(b, chars) >= 0; }

// ContainsRune reports whether the rune is contained in the UTF-8-encoded byte slice b.
inline bool ContainsRune(const slice<byte>& b, rune r) { return IndexRune(b, r) >= 0; }

}  // namespace bytes
}  // namespace gx
