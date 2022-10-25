//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "builder.h"

namespace gx {
namespace strings {

// IndexByte returns the index of the first instance of c in s, or -1 if c is not present in s.
int IndexByte(const string& s, byte c);

// Index returns the index of the first instance of substr in s, or -1 if substr is not present in s.
int Index(const string& s, const string& substr);

// LastIndexByte returns the index of the last instance of c in s, or -1 if c is not present in s.
int LastIndexByte(const string& s, byte c);

// LastIndex returns the index of the last instance of substr in s, or -1 if substr is not present in s.
int LastIndex(const string& s, const string& substr);

// Count counts the number of non-overlapping instances of substr in s.
// If substr is an empty string, Count returns 1 + the number of Unicode code points in s.
int Count(const string& s, const string& substr);

// Contains reports whether substr is within s.
inline bool Contains(const string& s, const string& substr) { return Index(s, substr) >= 0; }

// IndexAny returns the index of the first instance of any Unicode code point
// from chars in s, or -1 if no Unicode code point from chars is present in s.
inline int IndexAny(const string& s, const string& chars) { return Index(s, chars); }

namespace xx {
// Generic split: splits after each instance of sep,
// including sepSave bytes of sep in the subarrays.
Vec<string> genSplit(const string& s, const string& sep, int sepSave, int n);
}  // namespace xx

// SplitN slices s into substrings separated by sep and returns a slice of
// the substrings between those separators.
//
// The count determines the number of substrings to return:
//
//	n > 0: at most n substrings; the last substring will be the unsplit remainder.
//	n == 0: the result is nil (zero substrings)
//	n < 0: all substrings
//
// Edge cases for s and sep (for example, empty strings) are handled
// as described in the documentation for Split.
//
// To split around the first instance of a separator, see Cut.
inline Vec<string> SplitN(const string& s, const string& sep, int n) { return xx::genSplit(s, sep, 0, n); }

// SplitAfterN slices s into substrings after each instance of sep and
// returns a slice of those substrings.
//
// The count determines the number of substrings to return:
//
//	n > 0: at most n substrings; the last substring will be the unsplit remainder.
//	n == 0: the result is nil (zero substrings)
//	n < 0: all substrings
//
// Edge cases for s and sep (for example, empty strings) are handled
// as described in the documentation for SplitAfter.
inline Vec<string> SplitAfterN(const string& s, const string& sep, int n) { return xx::genSplit(s, sep, len(sep), n); }

// Split slices s into all substrings separated by sep and returns a slice of
// the substrings between those separators.
//
// If s does not contain sep and sep is not empty, Split returns a
// slice of length 1 whose only element is s.
//
// If sep is empty, Split splits after each UTF-8 sequence. If both s
// and sep are empty, Split returns an empty slice.
//
// It is equivalent to SplitN with a count of -1.
//
// To split around the first instance of a separator, see Cut.
inline Vec<string> Split(const string& s, const string& sep) { return xx::genSplit(s, sep, 0, -1); }

// SplitAfter ...
inline Vec<string> SplitAfter(const string& s, const string& sep) { return xx::genSplit(s, sep, len(sep), -1); }

// Repeat returns a new string consisting of count copies of the string s.
//
// It panics if count is negative or if
// the result of (len(s) * count) overflows.
string Repeat(const string& s, int count);

namespace xx {
bool cutsetMatch(const string& cutset, char v);
int cutsetLeftIndex(const string& s, const string& cutset);
int cutsetRightIndex(const string& s, const string& cutset);
int indexFunc(const string& s, const func<bool(rune)>& f, bool truth);
int lastIndexFunc(const string& s, const func<bool(rune)>& f, bool truth);
}  // namespace xx

// TrimLeftFunc returns a slice of the string s with all leading
// Unicode code points c satisfying f(c) removed.
string TrimLeftFunc(const string& s, const func<bool(rune)>& f);

// TrimRightFunc returns a slice of the string s with all trailing
// Unicode code points c satisfying f(c) removed.
string TrimRightFunc(const string& s, const func<bool(rune)>& f);

// TrimFunc returns a slice of the string s with all leading
// and trailing Unicode code points c satisfying f(c) removed.
inline string TrimFunc(const string& s, const func<bool(rune)>& f) { return TrimRightFunc(TrimLeftFunc(s, f), f); }

// IndexFunc returns the index into s of the first Unicode
// code point satisfying f(c), or -1 if none do.
inline int IndexFunc(const string& s, const func<bool(rune)>& f) { return xx::indexFunc(s, f, true); }

// LastIndexFunc returns the index into s of the last
// Unicode code point satisfying f(c), or -1 if none do.
inline int LastIndexFunc(const string& s, const func<bool(rune)>& f) { return xx::lastIndexFunc(s, f, true); }

// TrimLeft returns a slice of the string s with all leading
// Unicode code points contained in cutset removed.
//
// To remove a prefix, use TrimPrefix instead.
string TrimLeft(const string& s, const string& cutset);

// Trim returns a slice of the string s with all leading and
// trailing Unicode code points contained in cutset removed.
string Trim(const string& s, const string& cutset);

// TrimRight returns a slice of the string s, with all trailing
// Unicode code points contained in cutset removed.
//
// To remove a suffix, use TrimSuffix instead.
string TrimRight(const string& s, const string& cutset);

// TrimSpace returns a slice of the string s, with all leading
// and trailing white space removed, as defined by Unicode.
inline string TrimSpace(const string& s) { return Trim(s, " \t\r\n"); }

// HasPrefix tests whether the string s begins with prefix.
bool HasPrefix(const string& s, const string& prefix);

// HasSuffix tests whether the string s ends with suffix.
bool HasSuffix(const string& s, const string& suffix);

// TrimPrefix returns s without the provided leading prefix string.
// If s doesn't start with prefix, s is returned unchanged.
string TrimPrefix(const string& s, const string& prefix);

// TrimSuffix returns s without the provided trailing suffix string.
// If s doesn't end with suffix, s is returned unchanged.
string TrimSuffix(const string& s, const string& suffix);

// Replace returns a copy of the string s with the first n
// non-overlapping instances of old replaced by new.
// If old is empty, it matches at the beginning of the string
// and after each UTF-8 sequence, yielding up to k+1 replacements
// for a k-rune string.
// If n < 0, there is no limit on the number of replacements.
string Replace(const string& s, const string& olds, const string& news, int n);

// ReplaceAll returns a copy of the string s with all
// non-overlapping instances of old replaced by new.
// If old is empty, it matches at the beginning of the string
// and after each UTF-8 sequence, yielding up to k+1 replacements
// for a k-rune string.
inline string ReplaceAll(const string& s, const string& olds, const string& news) { return Replace(s, olds, news, -1); }

// EqualFold reports whether s and t, interpreted as UTF-8 strings,
// are equal under simple Unicode case-folding, which is a more general
// form of case-insensitivity.
bool EqualFold(const string& s, const string& t);

// Map returns a copy of the string s with all its characters modified
// according to the mapping function. If mapping returns a negative value, the character is
// dropped from the string with no replacement.
string Map(const func<void(char&)>& mapping, const string& s);

// ToLower returns s with all Unicode letters mapped to their lower case.
string ToLower(const string& s);

// ToUpper returns s with all Unicode letters mapped to their upper case.
string ToUpper(const string& s);

// Cut slices s around the first instance of sep,
// returning the text before and after sep.
// The found result reports whether sep appears in s.
// If sep does not appear in s, cut returns s, "", false.
R<string /*before*/, string /*after*/, bool /*found*/> Cut(const string& s, const string& sep);

// Fields splits the string s around each instance of one or more consecutive white space
// characters, as defined by unicode.IsSpace, returning a slice of substrings of s or an
// empty slice if s contains only white space.
stringz<> Fields(const string& s);

// FieldsFunc splits the string s at each run of Unicode code points c satisfying f(c)
// and returns an array of slices of s. If all code points in s satisfy f(c) or the
// string is empty, an empty slice is returned.
//
// FieldsFunc makes no guarantees about the order in which it calls f(c)
// and assumes that f always returns the same value for a given c.
stringz<> FieldsFunc(const string& s, const func<bool(rune)>& f);

// Join concatenates the elements of its first argument to create a single string. The separator
// string sep is placed between elements in the resulting string.
string Join(stringz<> elems, const string& sep);

}  // namespace strings
}  // namespace gx

namespace gx {
namespace unitest {
void test_strings();
}  // namespace unitest
}  // namespace gx
