#ifndef WHATWG_URL_UTF_H
#define WHATWG_URL_UTF_H

#include <cstdint> // uint32_t, [char16_t, char32_t]
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
    static bool read_code_point(const char32_t*& first, const char32_t* last, uint32_t& code_point);
private:
    const static uint8_t k_U8_LEAD3_T1_BITS[16];
    const static uint8_t k_U8_LEAD4_T1_BITS[16];
};


//
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
//

// This function is a modified version of the ICU 61.1 library's
// U8_INTERNAL_NEXT_OR_SUB macro from include\unicode\utf8.h file.

inline
bool url_util::read_code_point(const char*& first, const char* last, uint32_t& c) {
    c = static_cast<uint8_t>(*first++);
    if (c & 0x80) {
        uint8_t __t = 0;
        if (first != last &&
            // fetch/validate/assemble all but last trail byte
            (c >= 0xE0 ?
                (c < 0xF0 ? // U+0800..U+FFFF except surrogates
                    k_U8_LEAD3_T1_BITS[c &= 0xF] & (1 << ((__t = static_cast<uint8_t>(*first)) >> 5)) &&
                    (__t &= 0x3F, 1)
                    : // U+10000..U+10FFFF
                    (c -= 0xF0) <= 4 &&
                    k_U8_LEAD4_T1_BITS[(__t = static_cast<uint8_t>(*first)) >> 4] & (1 << c) &&
                    (c = (c << 6) | (__t & 0x3F), ++first != last) &&
                    (__t = static_cast<uint8_t>(*first) - 0x80) <= 0x3F) &&
                // valid second-to-last trail byte
                (c = (c << 6) | __t, ++first != last)
                : // U+0080..U+07FF
                c >= 0xC2 && (c &= 0x1F, 1)) &&
            // last trail byte
            (__t = static_cast<uint8_t>(*first) - 0x80) <= 0x3F &&
            (c = (c << 6) | __t, ++first, 1)) {
            // valid utf-8
        } else {
            // ill-formed
            // c = 0xfffd;
            return false;
        }
    }
    return true;
}

// This function is a modified version of the ICU 61.1 library's
// U16_NEXT_OR_FFFD macro from include\unicode\utf16.h file.

inline
bool url_util::read_code_point(const char16_t*& first, const char16_t* last, uint32_t& c) {
    c = *first++;
    if (U16_IS_SURROGATE(c)) {
        uint16_t __c2;
        if (U16_IS_SURROGATE_LEAD(c) && first != last && U16_IS_TRAIL(__c2 = *first)) {
            ++first;
            c = U16_GET_SUPPLEMENTARY(c, __c2);
        } else {
            // c = 0xfffd;
            return false;
        }
    }
    return true;
}

inline
bool url_util::read_code_point(const char32_t*& first, const char32_t*, uint32_t& c) {
    // no conversion
    c = *(first++);
    // don't allow surogates (U+D800..U+DFFF) and too high values
    return c < 0xD800u || (c > 0xDFFFu && c <= 0x10FFFFu);
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
    if (!read_code_point(first, last, code_point)) {
        code_point = kUnicodeReplacementCharacter;
        return false;
    }
    return true;
}


} // namespace whatwg

#endif // WHATWG_URL_UTF_H
