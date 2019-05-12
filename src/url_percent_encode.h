// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
// This file contains portions of modified code from:
// https://cs.chromium.org/chromium/src/url/url_canon_internal.h
// Copyright 2013 The Chromium Authors. All rights reserved.
//

#ifndef WHATWG_URL_PERCENT_ENCODE_H
#define WHATWG_URL_PERCENT_ENCODE_H

#include "config.h"
#include "url_utf.h"
#include <array>
#include <type_traits>


namespace whatwg {
namespace detail {

enum CharsType : uint8_t {
    // to represent percent encode sets
    CHAR_PATH = 0x01,
    CHAR_USERINFO = 0x02,
    CHAR_FRAGMENT = 0x04,
    CHAR_QUERY = 0x08,
    // other charcter classes
    CHAR_HOST_FORBIDDEN = 0x10,
    CHAR_IPV4 = 0x20,
    CHAR_HEX = 0x40,
};


#ifdef WHATWG__CPP_17

class CodePointSets {
public:
    constexpr CodePointSets() {

        // This class fills 256 (0x100) bytes array - one byte for each 8-bit character.
        // Bits of each byte in the array identifies sets to witch character belongs.
        // For percent encode sets: if charcter is in set, then corresponding bit is cleared.
        // That means, if bit is cleared, then character requires percent encoding, otherwise
        // character can be left as is.
        // For other sets: if charcter is in set, then corresponding bit is set.

        // C0 control percent-encode set
        // https://url.spec.whatwg.org/#c0-control-percent-encode-set

        // First: set all bits, which represent non C0 control characters
        // and then clear specific bits below
        doSetBits(CHAR_FRAGMENT | CHAR_PATH | CHAR_USERINFO | CHAR_QUERY, 0x20, 0x7E);

        // fragment percent-encode set
        // https://url.spec.whatwg.org/#fragment-percent-encode-set
        doClearBits(CHAR_FRAGMENT | CHAR_PATH | CHAR_USERINFO,
            { 0x20, 0x22, 0x3C, 0x3E, 0x60 });

        // path percent-encode set
        // https://url.spec.whatwg.org/#path-percent-encode-set
        doClearBits(CHAR_PATH | CHAR_USERINFO,
            { 0x23, 0x3F, 0x7B, 0x7D });

        // userinfo percent-encode set
        // https://url.spec.whatwg.org/#userinfo-percent-encode-set
        doClearBits(CHAR_USERINFO,
            { 0x2F, 0x3A, 0x3B, 0x3D, 0x40, 0x5B, 0x5C, 0x5D, 0x5E, 0x7C });

        // query state percent-encode set
        // https://url.spec.whatwg.org/#query-state
        doClearBits(CHAR_QUERY,
            { 0x20, 0x22, 0x23, 0x3C, 0x3E });

        // Forbidden host code points: U+0000 NULL, U+0009 TAB, U+000A LF, U+000D CR,
        // U+0020 SPACE, U+0023 (#), U+0025 (%), U+002F (/), U+003A (:), U+003F (?),
        // U+0040 (@), U+005B ([), U+005C (\), or U+005D (]).
        // https://url.spec.whatwg.org/#forbidden-host-code-point
        doSetBits(CHAR_HOST_FORBIDDEN,
            { 0x00, 0x09, 0x0A, 0x0D, 0x20, 0x23, 0x25, 0x2F, 0x3A, 0x3F, 0x40, 0x5B, 0x5C, 0x5D });

        // Hex digits (also allowed in IPv4)
        doSetBits(CHAR_HEX | CHAR_IPV4, '0', '9');
        doSetBits(CHAR_HEX | CHAR_IPV4, 'A', 'F');
        doSetBits(CHAR_HEX | CHAR_IPV4, 'a', 'f');

        // Additional characters allowed in IPv4
        doSetBits(CHAR_IPV4, { '.', 'X', 'x' });
    }

    constexpr uint8_t operator[](size_t ind) const {
        return arr_[ind];
    }

    constexpr size_t size() const {
        return 0x100;
    }

private:
    uint8_t arr_[0x100] = {};

    constexpr void doClearBits(uint8_t cbit, std::initializer_list<uint8_t> clist) {
        for (auto c : clist)
            arr_[c] = arr_[c] & ~cbit;
    }

    constexpr void doSetBits(uint8_t cbit, std::initializer_list<uint8_t> clist) {
        for (auto c : clist)
            arr_[c] = arr_[c] | cbit;
    }

    constexpr void doSetBits(uint8_t cbit, uint8_t from, uint8_t to) {
        for (unsigned c = from; c <= to; ++c)
            arr_[c] = arr_[c] | cbit;
    }
};

inline constexpr CodePointSets kCharBitSets;

#else

extern const uint8_t kCharBitSets[0x100];

#endif

// ----------------------------------------------------------------------------
// Chars classification

// Check char or code point value is 8 bit (<=0xFF)

inline bool is8BitChar(char) {
    return true;
}

inline bool is8BitChar(unsigned char) {
    return true;
}

template <typename CharT>
inline bool is8BitChar(CharT c) {
    return c <= 0xFF;
}

// Check char is in predefined set

template <typename CharT>
inline bool isCharInSet(CharT c, CharsType charsType) {
    return is8BitChar(c) && (kCharBitSets[static_cast<unsigned char>(c)] & charsType) != 0;
}

template <typename CharT>
inline bool isIPv4Char(CharT c) {
    return isCharInSet(c, CHAR_IPV4);
}

template <typename CharT>
inline bool isHexChar(CharT c) {
    return isCharInSet(c, CHAR_HEX);
}

template <typename CharT>
inline bool isForbiddenHostChar(CharT c) {
    return isCharInSet(c, CHAR_HOST_FORBIDDEN);
}

// ----------------------------------------------------------------------------
// Hex digit conversion tables and functions

// Maps the hex numerical values 0x0 to 0xf to the corresponding ASCII digit
// that will be used to represent it.
extern const char kHexCharLookup[0x10];

// This lookup table allows fast conversion between ASCII hex letters and their
// corresponding numerical value. The 8-bit range is divided up into 8
// regions of 0x20 characters each. Each of the three character types (numbers,
// uppercase, lowercase) falls into different regions of this range. The table
// contains the amount to subtract from characters in that range to get at
// the corresponding numerical value.
//
// See HexDigitToValue for the lookup.
extern const char kCharToHexLookup[8];

// Assumes the input is a valid hex digit! Call isHexChar before using this.
inline unsigned char HexCharToValue(unsigned char c) {
    return c - kCharToHexLookup[c / 0x20];
}

// ----------------------------------------------------------------------------
// Percent decode

// Given a character after '%' at |*first| in the string, this will decode
// the escaped value and put it into |*unescaped_value| on success (returns
// true). On failure, this will return false, and will not write into
// |*unescaped_value|.
//
// |*first| will be updated to point after the last character of the escape
// sequence. On failure, |*first| will be unchanged.

template <typename CharT>
inline bool DecodeEscaped(const CharT*& first, const CharT* last, unsigned char& unescaped_value) {
    if (last - first < 2 ||
        !isHexChar(first[0]) || !isHexChar(first[1])) {
        // not enough or invalid hex digits
        return false;
    }

    // Valid escape sequence.
    const unsigned char uc1 = static_cast<unsigned char>(first[0]);
    const unsigned char uc2 = static_cast<unsigned char>(first[1]);
    unescaped_value = (HexCharToValue(uc1) << 4) + HexCharToValue(uc2);
    first += 2;
    return true;
}

// ----------------------------------------------------------------------------
// Percent encode

// Write a single character, escaped, to the output. This always escapes: it
// does no checking that thee character requires escaping.
// Escaping makes sense only 8 bit chars, so code works in all cases of
// input parameters (8/16bit).
template<typename UINCHAR, typename OUTCHAR>
inline void AppendEscapedChar(UINCHAR ch, std::basic_string<OUTCHAR>& output) {
    output.push_back('%');
    output.push_back(kHexCharLookup[(ch >> 4) & 0xf]);
    output.push_back(kHexCharLookup[ch & 0xf]);
}

// Writes the given character to the output as UTF-8, escaping ALL
// characters (even when they are ASCII). This does NO checking of the
// validity of the Unicode characters; the caller should ensure that the value
// it is appending is valid to append.
inline void AppendUTF8EscapedValue(unsigned char_value, std::string& output) {
    url_utf::append_utf8<std::string, AppendEscapedChar>(char_value, output);
}

// Writes the given character to the output as UTF-8, escaped. Call this
// function only when the input is wide. Returns true on success. Failure
// means there was some problem with the encoding, we'll still try to
// update the |*begin| pointer and add a placeholder character to the
// output so processing can continue.

template <typename CharT>
inline bool AppendUTF8EscapedChar(const CharT*& first, const CharT* last, std::string& output) {
    // url_util::read_utf_char(..) will handle invalid characters for us and give
    // us the kUnicodeReplacementCharacter, so we don't have to do special
    // checking after failure, just pass through the failure to the caller.
    uint32_t code_point;
    bool success = url_utf::read_utf_char(first, last, code_point);
    AppendUTF8EscapedValue(code_point, output);
    return success;
}

// Appends the given string to the output, escaping characters that do not
// match the given |charsType| in CharsType.

template<typename CharT>
void AppendStringOfType(const CharT* first, const CharT* last, CharsType charsType, std::string& output) {
    using UCharT = typename std::make_unsigned<CharT>::type;

    for (auto it = first; it < last; ) {
        const UCharT ch = static_cast<UCharT>(*it);
        if (ch >= 0x80) {
            // read_utf_char will fill the code point with kUnicodeReplacementCharacter
            // when the input is invalid, which is what we want.
            uint32_t code_point;
            url_utf::read_utf_char(it, last, code_point);
            AppendUTF8EscapedValue(code_point, output);
        }
        else {
            // Just append the 7-bit character, possibly escaping it.
            const unsigned char uch = static_cast<unsigned char>(ch);
            if (!isCharInSet(uch, charsType))
                AppendEscapedChar(uch, output);
            else
                output.push_back(uch);
            ++it;
        }
    }
}


} // namespace detail
} // namespace whatwg

#endif // WHATWG_URL_PERCENT_ENCODE_H
