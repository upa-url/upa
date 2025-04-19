// Copyright 2016-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
// This file contains portions of modified code from the ICU project.
// Copyright (c) 2016-2023 Unicode, Inc.
//

#ifndef UPA_URL_UTF_H
#define UPA_URL_UTF_H

#include "url_result.h"
#include <cstdint> // uint8_t, uint32_t
#include <string>
#include <string_view>


namespace upa {

class url_utf {
public:
    template <typename CharT>
    static constexpr detail::result_value<uint32_t> read_utf_char(const CharT*& first, const CharT* last) noexcept;

    template <typename CharT>
    static void read_char_append_utf8(const CharT*& it, const CharT* last, std::string& output);
    static void read_char_append_utf8(const char*& it, const char* last, std::string& output);

    template <class Output, void appendByte(unsigned char, Output&)>
    static void append_utf8(uint32_t code_point, Output& output);

    template <class Output>
    static void append_utf16(uint32_t code_point, Output& output);

    // Convert to utf-8 string
    static std::string to_utf8_string(const char16_t* first, const char16_t* last);
    static std::string to_utf8_string(const char32_t* first, const char32_t* last);

    // Invalid utf-8 bytes sequences are replaced with 0xFFFD character.
    static void check_fix_utf8(std::string& str);

    static int compare_by_code_units(const char* first1, const char* last1, const char* first2, const char* last2) noexcept;
protected:
    // low level
    static constexpr bool read_code_point(const char*& first, const char* last, uint32_t& code_point) noexcept;
    static constexpr bool read_code_point(const char16_t*& first, const char16_t* last, uint32_t& code_point) noexcept;
    static constexpr bool read_code_point(const char32_t*& first, const char32_t* last, uint32_t& code_point) noexcept;
private:
    // Replacement character (U+FFFD)
    static constexpr std::string_view kReplacementCharUtf8{ "\xEF\xBF\xBD" };

    // Following two arrays have values from corresponding macros in ICU 74.1 library's
    // include\unicode\utf8.h file.

    // Internal bit vector for 3-byte UTF-8 validity check, for use in U8_IS_VALID_LEAD3_AND_T1.
    // Each bit indicates whether one lead byte + first trail byte pair starts a valid sequence.
    // Lead byte E0..EF bits 3..0 are used as byte index,
    // first trail byte bits 7..5 are used as bit index into that byte.
    static constexpr uint8_t k_U8_LEAD3_T1_BITS[16] = {
        0x20, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x10, 0x30, 0x30
    };
    // Internal bit vector for 4-byte UTF-8 validity check, for use in U8_IS_VALID_LEAD4_AND_T1.
    // Each bit indicates whether one lead byte + first trail byte pair starts a valid sequence.
    // First trail byte bits 7..4 are used as byte index,
    // lead byte F0..F4 bits 2..0 are used as bit index into that byte.
    static constexpr uint8_t k_U8_LEAD4_T1_BITS[16] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00
    };
};


// The URL class (https://url.spec.whatwg.org/#url-class) in URL Standard uses
// USVString for text data. The USVString is sequence of Unicode scalar values
// (https://heycam.github.io/webidl/#idl-USVString). The Infra Standard defines
// how to convert into it: "To convert a JavaScript string into a scalar value
// string, replace any surrogates with U+FFFD"
// (https://infra.spec.whatwg.org/#javascript-string-convert).
//
// The url_utf::read_utf_char(..) function follows this conversion when reads UTF-16
// text (CharT = char16_t). When it reads UTF-32 text (CharT = char32_t) it additionally
// replaces code units > 0x10FFFF with U+FFFD.
//
// When it reads UTF-8 text (CharT = char) it replaces bytes in invalid UTF-8 sequences
// with U+FFFD. This corresponds to UTF-8 decode without BOM
// (https://encoding.spec.whatwg.org/#utf-8-decode-without-bom).
//
// This function reads one character from [first, last), places it's value to `code_point`
// and advances `first` to point to the next character.

template <typename CharT>
constexpr detail::result_value<uint32_t> url_utf::read_utf_char(const CharT*& first, const CharT* last) noexcept {
    // read_code_point always initializes code_point
    uint32_t code_point{};
    if (read_code_point(first, last, code_point))
        return { true, code_point };
    return { false, 0xFFFD }; // REPLACEMENT CHARACTER
}

namespace detail {
    template <typename CharT>
    inline void append_to_string(uint8_t c, std::basic_string<CharT>& str) {
        str.push_back(static_cast<CharT>(c));
    };
} // namespace detail

template <typename CharT>
inline void url_utf::read_char_append_utf8(const CharT*& it, const CharT* last, std::string& output) {
    const uint32_t code_point = read_utf_char(it, last).value;
    append_utf8<std::string, detail::append_to_string>(code_point, output);
}

inline void url_utf::read_char_append_utf8(const char*& it, const char* last, std::string& output) {
    uint32_t code_point; // NOLINT(cppcoreguidelines-init-variables)
    const char* start = it;
    if (read_code_point(it, last, code_point))
        output.append(start, it);
    else
        output.append(kReplacementCharUtf8);
}

// ------------------------------------------------------------------------
// The code bellow is based on the ICU 74.1 library's UTF macros in
// utf8.h, utf16.h and utf.h files.
//
// (c) 2016 and later: Unicode, Inc. and others.
// License & terms of use: https://www.unicode.org/copyright.html
//

// Decoding UTF-8, UTF-16, UTF-32

// Modified version of the U8_INTERNAL_NEXT_OR_SUB macro in utf8.h from ICU

constexpr bool url_utf::read_code_point(const char*& first, const char* last, uint32_t& c) noexcept {
    c = static_cast<uint8_t>(*first++);
    if (c & 0x80) {
        uint8_t tmp = 0;
        // NOLINTBEGIN(bugprone-assignment-in-if-condition)
        if (first != last &&
            // fetch/validate/assemble all but last trail byte
            (c >= 0xE0 ?
                (c < 0xF0 ? // U+0800..U+FFFF except surrogates
                    k_U8_LEAD3_T1_BITS[c &= 0xF] & (1 << ((tmp = static_cast<uint8_t>(*first)) >> 5)) &&
                    (tmp &= 0x3F, 1)
                    : // U+10000..U+10FFFF
                    (c -= 0xF0) <= 4 &&
                    k_U8_LEAD4_T1_BITS[(tmp = static_cast<uint8_t>(*first)) >> 4] & (1 << c) &&
                    (c = (c << 6) | (tmp & 0x3F), ++first != last) &&
                    (tmp = static_cast<uint8_t>(static_cast<uint8_t>(*first) - 0x80)) <= 0x3F) &&
                // valid second-to-last trail byte
                (c = (c << 6) | tmp, ++first != last)
                : // U+0080..U+07FF
                c >= 0xC2 && (c &= 0x1F, 1)) &&
            // last trail byte
            (tmp = static_cast<uint8_t>(static_cast<uint8_t>(*first) - 0x80)) <= 0x3F &&
            (c = (c << 6) | tmp, ++first, 1)) {
            // valid utf-8
        } else {
            // ill-formed
            // c = 0xfffd;
            return false;
        }
        // NOLINTEND(bugprone-assignment-in-if-condition)
    }
    return true;
}

namespace detail {
    // UTF-16

    // Is this code unit/point a surrogate (U+d800..U+dfff)?
    // Based on U_IS_SURROGATE in utf.h from ICU
    template <typename T>
    constexpr bool u16_is_surrogate(T c) noexcept {
        return (c & 0xfffff800) == 0xd800;
    }

    // Assuming c is a surrogate code point (u16_is_surrogate(c)),
    // is it a lead surrogate?
    // Based on U16_IS_SURROGATE_LEAD in utf16.h from ICU
    template <typename T>
    constexpr bool u16_is_surrogate_lead(T c) noexcept {
        return (c & 0x400) == 0;
    }

    // Is this code unit a lead surrogate (U+d800..U+dbff)?
    // Based on U16_IS_LEAD in utf16.h from ICU
    template <typename T>
    constexpr bool u16_is_lead(T c) noexcept {
        return (c & 0xfffffc00) == 0xd800;
    }

    // Is this code unit a trail surrogate (U+dc00..U+dfff)?
    // Based on U16_IS_TRAIL in utf16.h from ICU
    template <typename T>
    constexpr bool u16_is_trail(T c) noexcept {
        return (c & 0xfffffc00) == 0xdc00;
    }

    // Get a supplementary code point value (U+10000..U+10ffff)
    // from its lead and trail surrogates.
    // Based on U16_GET_SUPPLEMENTARY in utf16.h from ICU
    constexpr uint32_t u16_get_supplementary(uint32_t lead, uint32_t trail) noexcept {
        constexpr uint32_t u16_surrogate_offset = (0xd800 << 10UL) + 0xdc00 - 0x10000;
        return (lead << 10UL) + trail - u16_surrogate_offset;
    }
} // namespace detail

// Modified version of the U16_NEXT_OR_FFFD macro in utf16.h from ICU

constexpr bool url_utf::read_code_point(const char16_t*& first, const char16_t* last, uint32_t& c) noexcept {
    c = *first++;
    if (detail::u16_is_surrogate(c)) {
        if (detail::u16_is_surrogate_lead(c) && first != last && detail::u16_is_trail(*first)) {
            c = detail::u16_get_supplementary(c, *first);
            ++first;
        } else {
            // c = 0xfffd;
            return false;
        }
    }
    return true;
}

constexpr bool url_utf::read_code_point(const char32_t*& first, const char32_t*, uint32_t& c) noexcept {
    // no conversion
    c = *first++;
    // don't allow surogates (U+D800..U+DFFF) and too high values
    return c < 0xD800u || (c > 0xDFFFu && c <= 0x10FFFFu);
}


// Encoding to UTF-8, UTF-16

// Modified version of the U8_APPEND_UNSAFE macro in utf8.h from ICU
//
// It converts code_point to UTF-8 bytes sequence and calls appendByte function for each byte.
// It assumes a valid code point (https://infra.spec.whatwg.org/#scalar-value).

template <class Output, void appendByte(uint8_t, Output&)>
inline void url_utf::append_utf8(uint32_t code_point, Output& output) {
    if (code_point <= 0x7f) {
        appendByte(static_cast<uint8_t>(code_point), output);
    } else {
        if (code_point <= 0x7ff) {
            appendByte(static_cast<uint8_t>((code_point >> 6) | 0xc0), output);
        } else {
            if (code_point <= 0xffff) {
                appendByte(static_cast<uint8_t>((code_point >> 12) | 0xe0), output);
            } else {
                appendByte(static_cast<uint8_t>((code_point >> 18) | 0xf0), output);
                appendByte(static_cast<uint8_t>(((code_point >> 12) & 0x3f) | 0x80), output);
            }
            appendByte(static_cast<uint8_t>(((code_point >> 6) & 0x3f) | 0x80), output);
        }
        appendByte(static_cast<uint8_t>((code_point & 0x3f) | 0x80), output);
    }
}

// Modified version of the U16_APPEND_UNSAFE macro in utf16.h from ICU
//
// It converts code_point to UTF-16 code units sequence and appends to output.
// It assumes a valid code point (https://infra.spec.whatwg.org/#scalar-value).

template <class Output>
inline void url_utf::append_utf16(uint32_t code_point, Output& output) {
    if (code_point <= 0xffff) {
        output.push_back(static_cast<char16_t>(code_point));
    } else {
        output.push_back(static_cast<char16_t>((code_point >> 10) + 0xd7c0));
        output.push_back(static_cast<char16_t>((code_point & 0x3ff) | 0xdc00));
    }
}


} // namespace upa

#endif // UPA_URL_UTF_H
