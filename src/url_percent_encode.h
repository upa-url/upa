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
#include "int_cast.h"
#include "str_arg.h"
#include "url_utf.h"
#include <array>
#include <type_traits>


namespace whatwg {


class code_point_set {
public:
#ifdef WHATWG__CPP_17
    constexpr code_point_set(void (*fun)(code_point_set& self)) {
        fun(*this);
    }

    constexpr void copy(const code_point_set& other) {
        for (std::size_t i = 0; i < arr_size_; ++i)
            arr_[i] = other.arr_[i];
    }

    constexpr void exclude(uint8_t c) {
        arr_[c >> 3] &= ~(1u << (c & 0x07));
    }

    constexpr void include(uint8_t c) {
        arr_[c >> 3] |= (1u << (c & 0x07));
    }

    constexpr void exclude(std::initializer_list<uint8_t> clist) {
        for (auto c : clist)
            exclude(c);
    }

    constexpr void include(std::initializer_list<uint8_t> clist) {
        for (auto c : clist)
            include(c);
    }

    constexpr void include(uint8_t from, uint8_t to) {
        for (auto c = from; c <= to; ++c)
            include(c);
    }

    template <typename CharT>
    constexpr bool operator[](CharT c) const {
        const auto uc = whatwg::detail::to_unsigned(c);
        return is_8bit(uc) && (arr_[uc >> 3] & (1u << (uc & 0x07))) != 0;
    }

    // for dump program
    std::size_t arr_size() const { return arr_size_; }
    uint8_t arr_val(std::size_t i) const { return arr_[i]; }
#else

    template <typename CharT>
    bool operator[](CharT c) const {
        const auto uc = whatwg::detail::to_unsigned(c);
        return is_8bit(uc) && (arr_[uc >> 3] & (1u << (uc & 0x07))) != 0;
    }
#endif

private:
    // Check code point value is 8 bit (<=0xFF)
    static constexpr bool is_8bit(unsigned char) {
        return true;
    }

    template <typename CharT>
    static constexpr bool is_8bit(CharT c) {
        return c <= 0xFF;
    }

    // Data
    static const std::size_t arr_size_ = 32;
#ifdef WHATWG__CPP_17
    uint8_t arr_[arr_size_] = {};
#else
public:
    uint8_t arr_[arr_size_];
#endif
};


// Percent encode sets

#ifdef WHATWG__CPP_17

// If you edit following data, then please compile tools/dumpCharBitSets.cpp program
// with C++17 compiler, run it and copy output to the url_percent_encode.cpp file.
// This is required to support C++11, C++14 compilers.

// fragment percent-encode set
// https://url.spec.whatwg.org/#fragment-percent-encode-set
inline constexpr code_point_set fragment_no_encode_set([](code_point_set& self) constexpr {
    self.include(0x20, 0x7E); // C0 control percent-encode set
    self.exclude({ 0x20, 0x22, 0x3C, 0x3E, 0x60 });
    });

// query percent-encode set
// https://url.spec.whatwg.org/#query-percent-encode-set
inline constexpr code_point_set query_no_encode_set([](code_point_set& self) constexpr {
    self.include(0x20, 0x7E); // C0 control percent-encode set
    self.exclude({ 0x20, 0x22, 0x23, 0x3C, 0x3E });
    });

// special query percent-encode set
// https://url.spec.whatwg.org/#special-query-percent-encode-set
inline constexpr code_point_set special_query_no_encode_set([](code_point_set& self) constexpr {
    self.copy(query_no_encode_set);
    self.exclude(0x27);
    });

// path percent-encode set
// https://url.spec.whatwg.org/#path-percent-encode-set
inline constexpr code_point_set path_no_encode_set([](code_point_set& self) constexpr {
    self.copy(query_no_encode_set);
    self.exclude({ 0x3F, 0x60, 0x7B, 0x7D });
    });

// path percent-encode set with '%' (0x25)
inline constexpr code_point_set raw_path_no_encode_set([](code_point_set& self) constexpr {
    self.copy(path_no_encode_set);
    self.exclude(0x25);
    });

// userinfo percent-encode set
// https://url.spec.whatwg.org/#userinfo-percent-encode-set
inline constexpr code_point_set userinfo_no_encode_set([](code_point_set& self) constexpr {
    self.copy(path_no_encode_set);
    self.exclude({ 0x2F, 0x3A, 0x3B, 0x3D, 0x40, 0x5B, 0x5C, 0x5D, 0x5E, 0x7C });
    });

// component percent-encode set
// https://url.spec.whatwg.org/#component-percent-encode-set
inline constexpr code_point_set component_no_encode_set([](code_point_set& self) constexpr {
    self.copy(userinfo_no_encode_set);
    self.exclude({ 0x24, 0x25, 0x26, 0x2B, 0x2C });
    });

// Forbidden host code points: U+0000 NULL, U+0009 TAB, U+000A LF, U+000D CR,
// U+0020 SPACE, U+0023 (#), U+0025 (%), U+002F (/), U+003A (:), U+003C (<),
// U+003E (>), U+003F (?), U+0040 (@), U+005B ([), U+005C (\), U+005D (]), or
// U+005E (^).
// https://url.spec.whatwg.org/#forbidden-host-code-point
inline constexpr code_point_set host_forbidden_set([](code_point_set& self) constexpr {
    self.include({
        0x00, 0x09, 0x0A, 0x0D, 0x20, 0x23, 0x25, 0x2F, 0x3A, 0x3C, 0x3E, 0x3F, 0x40, 0x5B,
        0x5C, 0x5D, 0x5E });
    });

// Hex digits
inline constexpr code_point_set hex_digit_set([](code_point_set& self) constexpr {
    self.include('0', '9');
    self.include('A', 'F');
    self.include('a', 'f');
    });

// Characters allowed in IPv4
inline constexpr code_point_set ipv4_char_set([](code_point_set& self) constexpr {
    self.copy(hex_digit_set);
    self.include({ '.', 'X', 'x' });
    });

#else

// For C++11, C++14

extern const code_point_set fragment_no_encode_set;
extern const code_point_set query_no_encode_set;
extern const code_point_set special_query_no_encode_set;
extern const code_point_set path_no_encode_set;
extern const code_point_set raw_path_no_encode_set;
extern const code_point_set userinfo_no_encode_set;
extern const code_point_set component_no_encode_set;

extern const code_point_set host_forbidden_set;
extern const code_point_set hex_digit_set;
extern const code_point_set ipv4_char_set;

#endif


namespace detail {

// ----------------------------------------------------------------------------
// Check char is in predefined set

template <typename CharT>
inline bool isCharInSet(CharT c, const code_point_set& cpset) {
    return cpset[c];
}

template <typename CharT>
inline bool isIPv4Char(CharT c) {
    return isCharInSet(c, ipv4_char_set);
}

template <typename CharT>
inline bool isHexChar(CharT c) {
    return isCharInSet(c, hex_digit_set);
}

template <typename CharT>
inline bool isForbiddenHostChar(CharT c) {
    return isCharInSet(c, host_forbidden_set);
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
void AppendStringOfType(const CharT* first, const CharT* last, const code_point_set& cpset, std::string& output) {
    using UCharT = typename std::make_unsigned<CharT>::type;

    for (auto it = first; it < last; ) {
        const UCharT ch = static_cast<UCharT>(*it);
        if (ch >= 0x80) {
            // invalid utf-8/16/32 sequences will be replaced with kUnicodeReplacementCharacter
            AppendUTF8EscapedChar(it, last, output);
        } else {
            // Just append the 7-bit character, possibly escaping it.
            const unsigned char uch = static_cast<unsigned char>(ch);
            if (isCharInSet(uch, cpset)) {
                output.push_back(uch);
            } else {
                // other characters are escaped
                AppendEscapedChar(uch, output);
            }
            ++it;
        }
    }
}


} // namespace detail


/// Percent decode input string.
///
/// Invalid code points are replaced with U+FFFD characters.
///
/// More info:
/// https://url.spec.whatwg.org/#string-percent-decode
///
/// @param args[in] string input
/// @return percent decoded string
template <class ...Args, enable_if_str_arg_t<Args...> = 0>
inline std::string percent_decode(Args&&... args) {
    const auto inp = make_str_arg(std::forward<Args>(args)...);
    const auto* first = inp.begin();
    const auto* last = inp.end();

    std::string out;
    for (auto it = first; it != last;) {
        const auto uch = detail::to_unsigned(*it); ++it;
        if (uch < 0x80) {
            if (uch != '%') {
                out.push_back(static_cast<char>(uch));
                continue;
            }
            // uch == '%'
            unsigned char uc8;
            if (detail::DecodeEscaped(it, last, uc8)) {
                if (uc8 < 0x80) {
                    out.push_back(static_cast<char>(uc8));
                    continue;
                }
                // percent encoded utf-8 sequence
                std::string buff_utf8;
                buff_utf8.push_back(uc8);
                while (it != last && *it == '%') {
                    ++it; // skip '%'
                    if (!detail::DecodeEscaped(it, last, uc8))
                        uc8 = '%';
                    buff_utf8.push_back(uc8);
                }
                url_utf::check_fix_utf8(buff_utf8);
                out += buff_utf8;
                continue;
            }
            // detected invalid percent encoding
            out.push_back('%');
        } else { // uch >= 0x80
            uint32_t code_point;
            auto start = --it;
            if (url_utf::read_utf_char(it, last, code_point))
                out.append(start, it);
            else
                out.append("\xEF\xBF\xBD"); // REPLACEMENT CHARATCTER
        }
    }
    return out;
}

/// UTF-8 percent encode input string using specified percent encode set.
///
/// Invalid code points are replaced with UTF-8 percent encoded U+FFFD characters.
///
/// More info:
/// https://url.spec.whatwg.org/#string-utf-8-percent-encode
///
/// @param[in] no_encode_set percent no encode set
/// @param[in] args string input
/// @returns percent encoded string
template <class ...Args, enable_if_str_arg_t<Args...> = 0>
inline std::string percent_encode(const code_point_set& no_encode_set, Args&&... args) {
    const auto inp = make_str_arg(std::forward<Args>(args)...);

    std::string out;
    detail::AppendStringOfType(inp.begin(), inp.end(), no_encode_set, out);
    return out;
}

/// UTF-8 percent encode input string using component percent encode set.
///
/// Invalid code points are replaced with UTF-8 percent encoded U+FFFD characters.
///
/// More info:
/// https://url.spec.whatwg.org/#string-utf-8-percent-encode
/// https://url.spec.whatwg.org/#component-percent-encode-set
///
/// @param[in] args string input
/// @returns percent encoded string
template <class ...Args, enable_if_str_arg_t<Args...> = 0>
inline std::string encode_url_component(Args&&... args) {
    return percent_encode(component_no_encode_set, std::forward<Args>(args)...);
}


} // namespace whatwg

#endif // WHATWG_URL_PERCENT_ENCODE_H
