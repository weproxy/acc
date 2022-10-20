//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "letter.h"

namespace gx {
namespace unicode {

// IsSpace reports whether the rune is a space character as defined
// by Unicode's White Space property; in the Latin-1 space
// this is
//
//	'\t', '\n', '\v', '\f', '\r', ' ', U+0085 (NEL), U+00A0 (NBSP).
//
// Other definitions of spacing characters are set by category
// Z and property Pattern_White_Space.
inline bool IsSpace(rune r) {
    // This property isn't the same as Z; special-case it.
    if (uint32(r) <= MaxLatin1) {
        switch (r) {
            case '\t':
            case '\n':
            case '\v':
            case '\f':
            case '\r':
            case ' ':
            case 0x85:
            case 0xA0:
                return true;
        }
        return false;
    }
    return xx::isExcludingLatin(White_Space, r);
}

// IsSymbol reports whether the rune is a symbolic character.
inline bool IsSymbol(rune r) {
    // if (uint32(r) <= MaxLatin1) {
    //     return properties[uint8(r)] & pS != 0;
    // }
    // return xx::isExcludingLatin(Symbol, r);
    return true;
}

}  // namespace unicode
}  // namespace gx
