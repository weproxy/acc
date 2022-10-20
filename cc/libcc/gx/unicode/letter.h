//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "tables.h"

namespace gx {
namespace unicode {

constexpr int MaxRune = 0x0010FFFF;      // Maximum valid Unicode code point.
constexpr int ReplacementChar = 0xFFFD;  // Represents invalid code points.
constexpr int MaxASCII = 0x007F;         // maximum ASCII value.
constexpr int MaxLatin1 = 0x00FF;        // maximum Latin-1 value.

namespace xx {

// isExcludingLatin ...
inline bool isExcludingLatin(const RangeTable& rangeTab, rune r )  {
	// r16 := rangeTab.R16
	// // Compare as uint32 to correctly handle negative runes.
	// if off := rangeTab.LatinOffset; len(r16) > off && uint32(r) <= uint32(r16[len(r16)-1].Hi) {
	// 	return is16(r16[off:], uint16(r))
	// }
	// r32 := rangeTab.R32
	// if len(r32) > 0 && r >= rune(r32[0].Lo) {
	// 	return is32(r32, uint32(r))
	// }
	return false;
}

} // namespace xx

}  // namespace unicode
}  // namespace gx
