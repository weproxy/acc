//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"

namespace gx {
namespace unicode {

// Range16 represents of a range of 16-bit Unicode code points. The range runs from Lo to Hi
// inclusive and has the specified stride.
struct Range16 {
    uint16 Lo;
    uint16 Hi;
    uint16 Stride;
};

// Range32 represents of a range of Unicode code points and is used when one or
// more of the values will not fit in 16 bits. The range runs from Lo to Hi
// inclusive and has the specified stride. Lo and Hi must always be >= 1<<16.
struct Range32 {
    uint32 Lo;
    uint32 Hi;
    uint32 Stride;
};

// RangeTable defines a set of Unicode code points by listing the ranges of
// code points within the set. The ranges are listed in two slices
// to save space: a slice of 16-bit ranges and a slice of 32-bit ranges.
// The two slices must be in sorted order and non-overlapping.
// Also, R32 should contain only values >= 0x10000 (1<<16).
struct RangeTable {
    slice<Range16> R16;
    slice<Range32> R32;
    int LatinOffset{0};  // number of entries in R16 with Hi <= MaxLatin1
};

////////////////////////////////////////////////////////////////////////////////
//
extern RangeTable _S;

extern RangeTable& Symbol; // Symbol/S is the set of Unicode symbol characters, category S.

extern RangeTable White_Space;

}  // namespace unicode
}  // namespace gx
