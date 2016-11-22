#ifndef WHATWG_URL_UTIL_H
#define WHATWG_URL_UTIL_H

#include <cstdint>
// ICU
#include "unicode/utf.h"

namespace whatwg {

class url_util {
public:
    template <typename CharT>
    static bool read_utf_char(const CharT*& first, const CharT* last, uint32_t& code_point);
protected:
    // low level
    static bool read_code_point(const char*& first, const char* last, uint32_t& code_point);
    static bool read_code_point(const char16_t*& first, const char16_t* last, uint32_t& code_point);
};

// https://cs.chromium.org/chromium/src/base/strings/utf_string_conversion_utils.cc

inline bool IsValidCodepoint(uint32_t code_point) {
    // Excludes the surrogate code points ([0xD800, 0xDFFF]) and
    // codepoints larger than 0x10FFFF (the highest codepoint allowed).
    // Non-characters and unassigned codepoints are allowed.
    return code_point < 0xD800u ||
        (code_point >= 0xE000u && code_point <= 0x10FFFFu);
}

inline bool IsValidCharacter(uint32_t code_point) {
    // Excludes non-characters (U+FDD0..U+FDEF, and all codepoints ending in
    // 0xFFFE or 0xFFFF) from the set of valid code points.
    return code_point < 0xD800u || (code_point >= 0xE000u &&
        code_point < 0xFDD0u) || (code_point > 0xFDEFu &&
        code_point <= 0x10FFFFu && (code_point & 0xFFFEu) != 0xFFFEu);
}

inline 
bool url_util::read_code_point(const char*& first, const char* last, uint32_t& code_point) {
    int32_t i = 0;
    int32_t length = last - first;
    U8_NEXT(first, i, length, code_point);
    first += i;
    return IsValidCodepoint(code_point);
}

inline
bool url_util::read_code_point(const char16_t*& first, const char16_t* last, uint32_t& code_point) {
    if (U16_IS_SURROGATE(first[0])) {
        if (!U16_IS_SURROGATE_LEAD(first[0]) ||
            first + 1 >= last ||
            !U16_IS_TRAIL(first[1])) {
            // Invalid surrogate pair.
            first++; // MANO: paimam vienà
            return false;
        }
        // Valid surrogate pair.
        code_point = U16_GET_SUPPLEMENTARY(first[0], first[1]);
        first += 2;
    } else {
        // Not a surrogate, just one 16-bit word.
        code_point = first[0];
        first++;
    }
    return IsValidCodepoint(code_point);
}

// https://cs.chromium.org/chromium/src/url/url_canon_internal.h
// https://cs.chromium.org/chromium/src/url/url_canon_internal.cc
// ReadUTFChar(..)

#define kUnicodeReplacementCharacter 0xfffd

// Reads one character in UTF-8 / UTF-16 in |first| and places
// the decoded value into |code_point|. If the character is valid, we will
// return true. If invalid, we'll return false and put the
// kUnicodeReplacementCharacter into |code_point|.
//
// |first| will be updated to point to the next character

template <typename CharT>
inline bool url_util::read_utf_char(const CharT*& first, const CharT* last, uint32_t& code_point) {
    if (!read_code_point(first, last, code_point) ||
        !IsValidCharacter(code_point))
    {
        code_point = kUnicodeReplacementCharacter;
        return false;
    }
    return true;
}


} // namespace whatwg

#endif // WHATWG_URL_UTIL_H
