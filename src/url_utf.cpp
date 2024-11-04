// Copyright 2016-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url_utf.h"
#include <cassert>

namespace upa {

template <typename CharT>
inline std::string to_utf8_stringT(const CharT* first, const CharT* last) {
    std::string output;
    for (auto it = first; it < last;) {
        url_utf::read_char_append_utf8(it, last, output);
    }
    return output;
}

std::string url_utf::to_utf8_string(const char16_t* first, const char16_t* last) {
    return to_utf8_stringT(first, last);
}
std::string url_utf::to_utf8_string(const char32_t* first, const char32_t* last) {
    return to_utf8_stringT(first, last);
}

void url_utf::check_fix_utf8(std::string& str) {
    const char* first = str.data();
    const char* last = str.data() + str.length();

    uint32_t code_point; // NOLINT(cppcoreguidelines-init-variables)
    const char* ptr = first;
    const char* it = first;
    while (it != last && read_code_point(it, last, code_point))
        ptr = it;

    if (ptr != last) {
        // replace invalid UTF-8 byte sequences with replacement char
        std::string buff;
        buff.append(first, ptr);
        buff.append(kReplacementCharUtf8);

        const char* bgn = it;
        ptr = it;
        while (it != last) {
            if (read_code_point(it, last, code_point)) {
                ptr = it;
            } else {
                buff.append(bgn, ptr);
                buff.append(kReplacementCharUtf8);
                bgn = it;
                ptr = it;
            }
        }
        buff.append(bgn, ptr);
        str = std::move(buff);
    }
}

int url_utf::compare_by_code_units(const char* first1, const char* last1, const char* first2, const char* last2) noexcept {
    const auto* it1 = first1;
    const auto* it2 = first2;
    while (it1 != last1 && it2 != last2) {
        if (static_cast<unsigned char>(*it1) < 0x80 || static_cast<unsigned char>(*it2) < 0x80) {
            if (*it1 == *it2) {
                ++it1, ++it2;
                continue;
            }
            return
                static_cast<int>(static_cast<unsigned char>(*it1)) -
                static_cast<int>(static_cast<unsigned char>(*it2));
        }

        // read code points
        const uint32_t cp1 = read_utf_char(it1, last1).value;
        const uint32_t cp2 = read_utf_char(it2, last2).value;
        if (cp1 == cp2) continue;

        // code points not equal - compare code units
        uint32_t cu1 = cp1 <= 0xffff ? cp1 : ((cp1 >> 10) + 0xd7c0);
        uint32_t cu2 = cp2 <= 0xffff ? cp2 : ((cp2 >> 10) + 0xd7c0);
        // cu1 can be equal to cu2 if they both are lead surrogates
        if (cu1 == cu2) {
            assert(detail::u16_is_lead(cu1));
            // get trail surrogates
            cu1 = (cp1 & 0x3ff);// | 0xdc00;
            cu2 = (cp2 & 0x3ff);// | 0xdc00;
        }
        return static_cast<int>(cu1) - static_cast<int>(cu2);
    }
    if (it1 != last1) return 1;
    if (it2 != last2) return -1;
    return 0;
}

} // namespace upa
