// Copyright 2016-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
// This file contains portions of modified code from:
// https://cs.chromium.org/chromium/src/url/url_canon_internal.h
// Copyright 2013 The Chromium Authors. All rights reserved.
//

#ifndef UPA_URL_PERCENT_ENCODE_H
#define UPA_URL_PERCENT_ENCODE_H

#include "str_arg.h"
#include "url_utf.h"
#include "util.h"
#include <array>
#include <cstdint> // uint8_t
#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>


namespace upa {


/// @brief Represents code point set
///
/// Is used to define percent encode sets, forbidden host code points, and
/// other code point sets.
///
class code_point_set {
public:
    /// @brief constructor for code point set initialization
    ///
    /// Function @a fun must iniatialize @a self object by using code_point_set
    /// member functions: copy(), exclude(), and include().
    ///
    /// @param[in] fun constexpr function to initialize code point set elements
    constexpr explicit code_point_set(void (*fun)(code_point_set& self)) {
        fun(*this);
    }

    /// @brief copy code points from @a other set
    /// @param[in] other code point set to copy from
    constexpr void copy(const code_point_set& other) {
        arr_ = other.arr_;
    }

    /// @brief exclude @a c code point from set
    /// @param[in] c code point to exclude
    constexpr void exclude(uint8_t c) {
        arr_[c >> 3] &= ~(1u << (c & 0x07));
    }

    /// @brief include @a c code point to set
    /// @param[in] c code point to include
    constexpr void include(uint8_t c) {
        arr_[c >> 3] |= (1u << (c & 0x07));
    }

    /// @brief exclude list of code points from set
    /// @param[in] clist list of code points to exclude
    constexpr void exclude(std::initializer_list<uint8_t> clist) {
        for (auto c : clist)
            exclude(c);
    }

    /// @brief include code points from list
    /// @param[in] clist list of code points to include
    constexpr void include(std::initializer_list<uint8_t> clist) {
        for (auto c : clist)
            include(c);
    }

    /// @brief include range of code points to set
    /// @param[in] from,to range of code points to include
    constexpr void include(uint8_t from, uint8_t to) {
        for (auto c = from; c <= to; ++c)
            include(c);
    }

    /// @brief test code point set contains code point @a c
    /// @param[in] c code point to test
    template <typename CharT>
    [[nodiscard]] constexpr bool operator[](CharT c) const {
        const auto uc = util::to_unsigned(c);
        return is_8bit(uc) && (arr_[uc >> 3] & (1u << (uc & 0x07))) != 0;
    }

private:
    // Check code point value is 8 bit (<=0xFF)
    static constexpr bool is_8bit(unsigned char) noexcept {
        return true;
    }

    template <typename CharT>
    static constexpr bool is_8bit(CharT c) noexcept {
        return c <= 0xFF;
    }

    // Data
    std::array<uint8_t, 32> arr_{};
};


// Percent encode sets

// If you edit following data, then please compile tools/dumpCharBitSets.cpp program
// with C++17 compiler, run it and copy output to the url_percent_encode.cpp file.
// This is required to support C++11, C++14 compilers.

// fragment percent-encode set
// https://url.spec.whatwg.org/#fragment-percent-encode-set
inline constexpr code_point_set fragment_no_encode_set{ [](code_point_set& self) constexpr {
    self.include(0x20, 0x7E); // C0 control percent-encode set
    self.exclude({ 0x20, 0x22, 0x3C, 0x3E, 0x60 });
    } };

// query percent-encode set
// https://url.spec.whatwg.org/#query-percent-encode-set
inline constexpr code_point_set query_no_encode_set{ [](code_point_set& self) constexpr {
    self.include(0x20, 0x7E); // C0 control percent-encode set
    self.exclude({ 0x20, 0x22, 0x23, 0x3C, 0x3E });
    } };

// special query percent-encode set
// https://url.spec.whatwg.org/#special-query-percent-encode-set
inline constexpr code_point_set special_query_no_encode_set{ [](code_point_set& self) constexpr {
    self.copy(query_no_encode_set);
    self.exclude(0x27);
    } };

// path percent-encode set
// https://url.spec.whatwg.org/#path-percent-encode-set
inline constexpr code_point_set path_no_encode_set{ [](code_point_set& self) constexpr {
    self.copy(query_no_encode_set);
    self.exclude({ 0x3F, 0x60, 0x7B, 0x7D });
    } };

// path percent-encode set with '%' (0x25)
inline constexpr code_point_set raw_path_no_encode_set{ [](code_point_set& self) constexpr {
    self.copy(path_no_encode_set);
    self.exclude(0x25);
    } };

// POSIX path percent-encode set
// Additionally encode ':', '|' and '\' to prevent windows drive letter detection and interpretation of the
// '\' as directory separator in POSIX paths (for example "/c:\end" will be encoded to "/c%3A%5Cend").
inline constexpr code_point_set posix_path_no_encode_set{ [](code_point_set& self) constexpr {
    self.copy(raw_path_no_encode_set);
    self.exclude({ 0x3A, 0x5C, 0x7C }); // ':' (0x3A), '\' (0x5C), '|' (0x7C)
    } };

// userinfo percent-encode set
// https://url.spec.whatwg.org/#userinfo-percent-encode-set
inline constexpr code_point_set userinfo_no_encode_set{ [](code_point_set& self) constexpr {
    self.copy(path_no_encode_set);
    self.exclude({ 0x2F, 0x3A, 0x3B, 0x3D, 0x40, 0x5B, 0x5C, 0x5D, 0x5E, 0x7C });
    } };

// component percent-encode set
// https://url.spec.whatwg.org/#component-percent-encode-set
inline constexpr code_point_set component_no_encode_set{ [](code_point_set& self) constexpr {
    self.copy(userinfo_no_encode_set);
    self.exclude({ 0x24, 0x25, 0x26, 0x2B, 0x2C });
    } };


namespace detail {

// Code point sets in one bytes array

enum CP_SET : std::uint8_t {
    ASCII_DOMAIN_SET = 0x01,
    DOMAIN_FORBIDDEN_SET = 0x02,
    HOST_FORBIDDEN_SET = 0x04,
    HEX_DIGIT_SET = 0x08,
    IPV4_CHAR_SET = 0x10,
    SCHEME_SET = 0x20,
};

class code_points_multiset {
public:
    constexpr code_points_multiset() {
        // Forbidden host code points: U+0000 NULL, U+0009 TAB, U+000A LF, U+000D CR,
        // U+0020 SPACE, U+0023 (#), U+002F (/), U+003A (:), U+003C (<), U+003E (>),
        // U+003F (?), U+0040 (@), U+005B ([), U+005C (\), U+005D (]), U+005E (^) and
        // U+007C (|).
        // https://url.spec.whatwg.org/#forbidden-host-code-point
        include(static_cast<CP_SET>(HOST_FORBIDDEN_SET | DOMAIN_FORBIDDEN_SET), {
            0x00, 0x09, 0x0A, 0x0D, 0x20, 0x23, 0x2F, 0x3A, 0x3C, 0x3E, 0x3F, 0x40, 0x5B,
            0x5C, 0x5D, 0x5E, 0x7C });

        // Forbidden domain code points: forbidden host code points, C0 controls, U+0025 (%)
        // and U+007F DELETE.
        // https://url.spec.whatwg.org/#forbidden-domain-code-point
        include(DOMAIN_FORBIDDEN_SET, 0x00, 0x1F); // C0 controls
        include(DOMAIN_FORBIDDEN_SET, { 0x25, 0x7F });

        // ASCII domain code points

        // All ASCII excluding C0 controls (forbidden in domains)
        include(ASCII_DOMAIN_SET, 0x20, 0x7F);
        // exclude forbidden host code points
        exclude(ASCII_DOMAIN_SET, {
            0x00, 0x09, 0x0A, 0x0D, 0x20, 0x23, 0x2F, 0x3A, 0x3C, 0x3E, 0x3F, 0x40, 0x5B,
            0x5C, 0x5D, 0x5E, 0x7C });
        // exclude forbidden domain code points
        exclude(ASCII_DOMAIN_SET, { 0x25, 0x7F });

        // Hex digits
        include(static_cast<CP_SET>(HEX_DIGIT_SET | IPV4_CHAR_SET), '0', '9');
        include(static_cast<CP_SET>(HEX_DIGIT_SET | IPV4_CHAR_SET), 'A', 'F');
        include(static_cast<CP_SET>(HEX_DIGIT_SET | IPV4_CHAR_SET), 'a', 'f');

        // Characters allowed in IPv4
        include(IPV4_CHAR_SET, { '.', 'X', 'x' });

        // Scheme code points
        // ASCII alphanumeric, U+002B (+), U+002D (-), or U+002E (.)
        // https://url.spec.whatwg.org/#scheme-state
        include(SCHEME_SET, '0', '9');
        include(SCHEME_SET, 'A', 'Z');
        include(SCHEME_SET, 'a', 'z');
        include(SCHEME_SET, { 0x2B, 0x2D, 0x2E });
    }

    /// @brief test the @a cps code points set contains code point @a c
    /// @param[in] c code point to test
    /// @param[in] cps code point set
    template <typename CharT>
    [[nodiscard]] constexpr bool char_in_set(CharT c, CP_SET cps) const {
        const auto uc = util::to_unsigned(c);
        return is_8bit(uc) && (arr_[uc] & cps);
    }

private:
    /// @brief include @a c code point to @a cpsbits sets
    /// @param[in] cpsbits code points sets
    /// @param[in] c code point to include
    constexpr void include(CP_SET cpsbits, uint8_t c) {
        arr_[c] |= cpsbits;
    }

    /// @brief include code points from list
    /// @param[in] cpsbits code points sets
    /// @param[in] clist list of code points to include
    constexpr void include(CP_SET cpsbits, std::initializer_list<uint8_t> clist) {
        for (auto c : clist)
            include(cpsbits, c);
    }

    /// @brief include range of code points to set
    /// @param[in] cpsbits code points sets
    /// @param[in] from,to range of code points to include
    constexpr void include(CP_SET cpsbits, uint8_t from, uint8_t to) {
        for (auto c = from; c <= to; ++c)
            include(cpsbits, c);
    }

    /// @brief exclude @a c code point from set
    /// @param[in] cpsbits code points sets
    /// @param[in] c code point to exclude
    constexpr void exclude(CP_SET cpsbits, uint8_t c) {
        arr_[c] &= ~cpsbits;
    }

    /// @brief exclude list of code points from set
    /// @param[in] cpsbits code points sets
    /// @param[in] clist list of code points to exclude
    constexpr void exclude(CP_SET cpsbits, std::initializer_list<uint8_t> clist) {
        for (auto c : clist)
            exclude(cpsbits, c);
    }

    // Check code point value is 8 bit (<=0xFF)
    static constexpr bool is_8bit(unsigned char) noexcept {
        return true;
    }

    template <typename CharT>
    static constexpr bool is_8bit(CharT c) noexcept {
        return c <= 0xFF;
    }

    // Data
    std::array<uint8_t, 256> arr_{};
};

inline constexpr code_points_multiset code_points;

// ----------------------------------------------------------------------------
// Check char is in predefined set

template <typename CharT>
constexpr bool is_char_in_set(CharT c, const code_point_set& cpset) {
    return cpset[c];
}

template <typename CharT>
constexpr bool is_ipv4_char(CharT c) {
    return code_points.char_in_set(c, IPV4_CHAR_SET);
}

template <typename CharT>
constexpr bool is_hex_char(CharT c) {
    return code_points.char_in_set(c, HEX_DIGIT_SET);
}

template <typename CharT>
constexpr bool is_scheme_char(CharT c) {
    return code_points.char_in_set(c, SCHEME_SET);
}

template <typename CharT>
constexpr bool is_forbidden_domain_char(CharT c) {
    return code_points.char_in_set(c, DOMAIN_FORBIDDEN_SET);
}

template <typename CharT>
constexpr bool is_forbidden_host_char(CharT c) {
    return code_points.char_in_set(c, HOST_FORBIDDEN_SET);
}

template <typename CharT>
constexpr bool is_ascii_domain_char(CharT c) {
    return code_points.char_in_set(c, ASCII_DOMAIN_SET);
}

// Char classification

template <typename CharT>
constexpr bool is_ascii_digit(CharT ch) noexcept {
    return ch <= '9' && ch >= '0';
}

template <typename CharT>
constexpr bool is_ascii_alpha(CharT ch) noexcept {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
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
// See hex_char_to_num for the lookup.
extern const char kCharToHexLookup[8];

// Assumes the input is a valid hex digit! Call is_hex_char before using this.
inline unsigned char hex_char_to_num(unsigned char c) noexcept {
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
inline bool decode_hex_to_byte(const CharT*& first, const CharT* last, unsigned char& unescaped_value) {
    if (last - first < 2 ||
        !is_hex_char(first[0]) || !is_hex_char(first[1])) {
        // not enough or invalid hex digits
        return false;
    }

    // Valid escape sequence.
    const auto uc1 = static_cast<unsigned char>(first[0]);
    const auto uc2 = static_cast<unsigned char>(first[1]);
    unescaped_value = (hex_char_to_num(uc1) << 4) + hex_char_to_num(uc2);
    first += 2;
    return true;
}

// ----------------------------------------------------------------------------
// Percent encode

// Percent-encodes byte and appends to string
// See: https://url.spec.whatwg.org/#percent-encode

inline void append_percent_encoded_byte(unsigned char uc, std::string& output) {
    output.push_back('%');
    output.push_back(kHexCharLookup[uc >> 4]);
    output.push_back(kHexCharLookup[uc & 0xf]);
}

// Reads one character from string (first, last), converts to UTF-8, then
// percent-encodes, and appends to `output`. Replaces invalid UTF-8, UTF-16 or UTF-32
// sequences in input with Unicode replacement characters (U+FFFD) if present.

template <typename CharT>
inline bool append_utf8_percent_encoded_char(const CharT*& first, const CharT* last, std::string& output) {
    // url_util::read_utf_char(..) will handle invalid characters for us and give
    // us the kUnicodeReplacementCharacter, so we don't have to do special
    // checking after failure, just pass through the failure to the caller.
    const auto cp_res = url_utf::read_utf_char(first, last);
    // convert cp_res.value code point to UTF-8, then percent encode and append to `output`
    url_utf::append_utf8<std::string, append_percent_encoded_byte>(cp_res.value, output);
    return cp_res.result;
}

// Converts input string (first, last) to UTF-8, then percent encodes bytes not
// in `cpset`, and appends to `output`. Replaces invalid UTF-8, UTF-16 or UTF-32
// sequences in input with Unicode replacement characters (U+FFFD) if present.

template<typename CharT>
inline void append_utf8_percent_encoded(const CharT* first, const CharT* last, const code_point_set& cpset, std::string& output) {
    using UCharT = std::make_unsigned_t<CharT>;

    for (auto it = first; it < last; ) {
        const auto uch = static_cast<UCharT>(*it);
        if (uch >= 0x80) {
            // invalid utf-8/16/32 sequences will be replaced with kUnicodeReplacementCharacter
            append_utf8_percent_encoded_char(it, last, output);
        } else {
            // Just append the 7-bit character, possibly percent encoding it
            const auto uc = static_cast<unsigned char>(uch);
            if (is_char_in_set(uc, cpset)) {
                output.push_back(uc);
            } else {
                // other characters are percent encoded
                append_percent_encoded_byte(uc, output);
            }
            ++it;
        }
    }
}

/// @brief Percent decode input string and append to output string
///
/// Invalid code points are replaced with U+FFFD characters.
///
/// More info:
/// https://url.spec.whatwg.org/#string-percent-decode
///
/// @param[in] str string input
/// @param[out] output string output
template <class StrT, enable_if_str_arg_t<StrT> = 0>
inline void append_percent_decoded(StrT&& str, std::string& output) {
    const auto inp = make_str_arg(std::forward<StrT>(str));
    const auto* first = inp.begin();
    const auto* last = inp.end();

    for (auto it = first; it != last;) {
        const auto uch = util::to_unsigned(*it); ++it;
        if (uch < 0x80) {
            if (uch != '%') {
                output.push_back(static_cast<char>(uch));
                continue;
            }
            // uch == '%'
            unsigned char uc8; // NOLINT(cppcoreguidelines-init-variables)
            if (decode_hex_to_byte(it, last, uc8)) {
                if (uc8 < 0x80) {
                    output.push_back(static_cast<char>(uc8));
                    continue;
                }
                // percent encoded utf-8 sequence
                std::string buff_utf8;
                buff_utf8.push_back(static_cast<char>(uc8));
                while (it != last && *it == '%') {
                    ++it; // skip '%'
                    if (!decode_hex_to_byte(it, last, uc8))
                        uc8 = '%';
                    buff_utf8.push_back(static_cast<char>(uc8));
                }
                url_utf::check_fix_utf8(buff_utf8);
                output += buff_utf8;
                continue;
            }
            // detected invalid percent encoding
            output.push_back('%');
        } else { // uch >= 0x80
            --it;
            url_utf::read_char_append_utf8(it, last, output);
        }
    }
}


} // namespace detail


/// @brief Percent decode input string.
///
/// Invalid code points are replaced with U+FFFD characters.
///
/// More info:
/// https://url.spec.whatwg.org/#string-percent-decode
///
/// @param[in] str string input
/// @return percent decoded string
template <class StrT, enable_if_str_arg_t<StrT> = 0>
[[nodiscard]] inline std::string percent_decode(StrT&& str) {
    std::string out;
    detail::append_percent_decoded(std::forward<StrT>(str), out);
    return out;
}

/// @brief UTF-8 percent encode input string using specified percent encode set.
///
/// Invalid code points are replaced with UTF-8 percent encoded U+FFFD characters.
///
/// More info:
/// https://url.spec.whatwg.org/#string-utf-8-percent-encode
///
/// @param[in] str string input
/// @param[in] no_encode_set percent no encode set, contains code points which
///            must not be percent encoded
/// @return percent encoded string
template <class StrT, enable_if_str_arg_t<StrT> = 0>
[[nodiscard]] inline std::string percent_encode(StrT&& str, const code_point_set& no_encode_set) {
    const auto inp = make_str_arg(std::forward<StrT>(str));

    std::string out;
    detail::append_utf8_percent_encoded(inp.begin(), inp.end(), no_encode_set, out);
    return out;
}

/// @brief UTF-8 percent encode input string using component percent encode set.
///
/// Invalid code points are replaced with UTF-8 percent encoded U+FFFD characters.
///
/// More info:
/// * https://url.spec.whatwg.org/#string-utf-8-percent-encode
/// * https://url.spec.whatwg.org/#component-percent-encode-set
///
/// @param[in] str string input
/// @return percent encoded string
template <class StrT, enable_if_str_arg_t<StrT> = 0>
[[nodiscard]] inline std::string encode_url_component(StrT&& str) {
    return percent_encode(std::forward<StrT>(str), component_no_encode_set);
}


} // namespace upa

#endif // UPA_URL_PERCENT_ENCODE_H
