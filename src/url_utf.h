#ifndef WHATWG_URL_UTF_H
#define WHATWG_URL_UTF_H

#include "buffer.h"
#include <cstdint> // uint32_t, [char16_t, char32_t]
#include <string>


namespace whatwg {

class url_utf {
public:
    template <typename CharT>
    static bool read_utf_char(const CharT*& first, const CharT* last, uint32_t& code_point);

    template <class Output, void appendByte(unsigned char, Output&)>
    static void append_utf8(uint32_t code_point, Output& output);

    static void append_utf16(uint32_t code_point, simple_buffer<char16_t>& output);
protected:
    // low level
    static bool read_code_point(const char*& first, const char* last, uint32_t& code_point);
    static bool read_code_point(const char16_t*& first, const char16_t* last, uint32_t& code_point);
    static bool read_code_point(const char32_t*& first, const char32_t* last, uint32_t& code_point);
private:
    const static uint8_t k_U8_LEAD3_T1_BITS[16];
    const static uint8_t k_U8_LEAD4_T1_BITS[16];
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
// This function reads one charcter from [first, last), places it's value to `code_point`
// and advances `first` to point to the next character.

template <typename CharT>
inline bool url_utf::read_utf_char(const CharT*& first, const CharT* last, uint32_t& code_point) {
    if (!read_code_point(first, last, code_point)) {
        code_point = 0xFFFD; // REPLACEMENT CHARACTER
        return false;
    }
    return true;
}


// ------------------------------------------------------------------------
// The code bellow is based on the ICU 61.1 library's UTF macros in
// utf8.h, utf16.h and utf.h files.
//
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
//

// Decoding UTF-8, UTF-16, UTF-32

// Modified version of the U8_INTERNAL_NEXT_OR_SUB macro in utf8.h from ICU

inline bool url_utf::read_code_point(const char*& first, const char* last, uint32_t& c) {
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

namespace {
    // UTF-16

    // Is this code unit/point a surrogate (U+d800..U+dfff)?
    // Based on U_IS_SURROGATE in utf.h from ICU
    template <typename T>
    inline bool u16_is_surrogate(T c) {
        return (c & 0xfffff800) == 0xd800;
    }

    // Assuming c is a surrogate code point (u16_is_surrogate(c)),
    // is it a lead surrogate?
    // Based on U16_IS_SURROGATE_LEAD in utf16.h from ICU
    template <typename T>
    inline bool u16_is_surrogate_lead(T c) {
        return (c & 0x400) == 0;
    }

    // Is this code unit a trail surrogate (U+dc00..U+dfff)?
    // Based on U16_IS_TRAIL in utf16.h from ICU
    template <typename T>
    inline bool u16_is_trail(T c) {
        return (c & 0xfffffc00) == 0xdc00;
    }

    // Get a supplementary code point value (U+10000..U+10ffff)
    // from its lead and trail surrogates.
    // Based on U16_GET_SUPPLEMENTARY in utf16.h from ICU
    inline uint32_t u16_get_supplementary(uint32_t lead, uint32_t trail) {
        const uint32_t u16_surrogate_offset = (0xd800 << 10UL) + 0xdc00 - 0x10000;
        return (lead << 10UL) + trail - u16_surrogate_offset;
    }
}

// Modified version of the U16_NEXT_OR_FFFD macro in utf16.h from ICU

inline bool url_utf::read_code_point(const char16_t*& first, const char16_t* last, uint32_t& c) {
    c = *first++;
    if (u16_is_surrogate(c)) {
        uint16_t __c2;
        if (u16_is_surrogate_lead(c) && first != last && u16_is_trail(__c2 = *first)) {
            ++first;
            c = u16_get_supplementary(c, __c2);
        } else {
            // c = 0xfffd;
            return false;
        }
    }
    return true;
}

inline bool url_utf::read_code_point(const char32_t*& first, const char32_t*, uint32_t& c) {
    // no conversion
    c = *(first++);
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

// Modified version of the U16_APPEND_UNSAFE macro in utf8.h from ICU
//
// It converts code_point to UTF-16 code units sequence and appends to output.
// It assumes a valid code point (https://infra.spec.whatwg.org/#scalar-value).

inline void url_utf::append_utf16(uint32_t code_point, simple_buffer<char16_t>& output) {
    if (code_point <= 0xffff) {
        output.push_back(static_cast<char16_t>(code_point));
    } else {
        output.push_back(static_cast<char16_t>((code_point >> 10) + 0xd7c0));
        output.push_back(static_cast<char16_t>((code_point & 0x3ff) | 0xdc00));
    }
}


} // namespace whatwg

#endif // WHATWG_URL_UTF_H
