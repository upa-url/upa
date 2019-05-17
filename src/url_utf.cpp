// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url_utf.h"

namespace whatwg {

bool url_utf::convert_utf8_to_utf16(const char* first, const char* last, simple_buffer<char16_t>& output) {
    bool success = true;
    for (auto it = first; it < last;) {
        uint32_t code_point;
        success &= read_utf_char(it, last, code_point);
        append_utf16(code_point, output);
    }
    return success;
}

int url_utf::compare_by_code_units(const char* first1, const char* last1, const char* first2, const char* last2) {
    auto it1 = first1, it2 = first2;
    while (it1 != last1 && it2 != last2) {
        if (static_cast<unsigned char>(*it1) < 0x80 || static_cast<unsigned char>(*it2) < 0x80) {
            if (*it1 == *it2) {
                ++it1, ++it2;
                continue;
            }
            return int(static_cast<unsigned char>(*it1)) - int(static_cast<unsigned char>(*it2));
        }

        // read code points
        uint32_t cp1, cp2;
        read_utf_char(it1, last1, cp1);
        read_utf_char(it2, last2, cp2);
        if (cp1 == cp2) continue;

        // code points not equal - compare code units
        uint32_t cu1 = cp1 <= 0xffff ? cp1 : (cp1 >> 10);// +0xd7c0;
        uint32_t cu2 = cp2 <= 0xffff ? cp2 : (cp2 >> 10);// +0xd7c0;
        if (cu1 == cu2) {
            cu1 = (cp1 & 0x3ff);// | 0xdc00;
            cu2 = (cp2 & 0x3ff);// | 0xdc00;
        }
        return int(cu1) - int(cu2);
    }
    if (it1 != last1) return 1;
    if (it2 != last2) return -1;
    return 0;
}

//
// (c) 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
//

// Following two arrays have values from corresponding macros in ICU 61.1 library's
// include\unicode\utf8.h file.

// Internal bit vector for 3-byte UTF-8 validity check, for use in U8_IS_VALID_LEAD3_AND_T1.
// Each bit indicates whether one lead byte + first trail byte pair starts a valid sequence.
// Lead byte E0..EF bits 3..0 are used as byte index,
// first trail byte bits 7..5 are used as bit index into that byte.
const uint8_t url_utf::k_U8_LEAD3_T1_BITS[16] = { 0x20, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x10, 0x30, 0x30 };

// Internal bit vector for 4-byte UTF-8 validity check, for use in U8_IS_VALID_LEAD4_AND_T1.
// Each bit indicates whether one lead byte + first trail byte pair starts a valid sequence.
// First trail byte bits 7..4 are used as byte index,
// lead byte F0..F4 bits 2..0 are used as bit index into that byte.
const uint8_t url_utf::k_U8_LEAD4_T1_BITS[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00 };

}
