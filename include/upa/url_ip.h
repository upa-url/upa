// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_URL_IP_H
#define UPA_URL_IP_H

#include "url_percent_encode.h"
#include "url_result.h"
#include <algorithm>
#include <cassert>
#include <cstdint> // uint16_t, uint32_t, uint64_t
#include <limits>
#include <string>

namespace upa {

// append unsigned integer to string

template <typename UIntT>
inline void unsigned_to_str(UIntT num, std::string& output, UIntT base) {
    static const char digit[] = "0123456789abcdef";

    // count digits
    std::size_t count = output.length() + 1;
    // one division is needed to prevent the multiplication overflow
    const UIntT num0 = num / base;
    for (UIntT divider = 1; divider <= num0; divider *= base)
        ++count;
    output.resize(count);

    // convert
    do {
        output[--count] = digit[num % base];
        num /= base;
    } while (num);
}

// The hostname ends in a number checker
// https://url.spec.whatwg.org/#ends-in-a-number-checker
// Optimized version
//
template <typename CharT>
static inline bool hostname_ends_in_a_number(const CharT* first, const CharT* last) {
    if (first != last) {
        // if the last label is empty string, then skip it
        if (*(last - 1) == '.')
            --last;
        // find start of the label
        const CharT* start_of_label = last;
        while (start_of_label != first && *(start_of_label - 1) != '.')
            --start_of_label;
        // check for a number
        const auto len = last - start_of_label;
        if (len) {
            if (len >= 2 && start_of_label[0] == '0' && (start_of_label[1] == 'X' || start_of_label[1] == 'x')) {
                // "0x" is valid IPv4 number (std::all_of returns true if the range is empty)
                return std::all_of(start_of_label + 2, last, detail::is_hex_char<CharT>);
            }
            // decimal or octal number?
            return std::all_of(start_of_label, last, detail::is_ascii_digit<CharT>);
        }
    }
    return false;
}

// IPv4 number parser
// https://url.spec.whatwg.org/#ipv4-number-parser
//
// - on success sets number value and returns validation_errc::ok
// - if resulting number can not be represented by uint32_t value, then returns
//   validation_errc::ipv4_non_numeric_part
//
// TODO-WARN: validationError
//
template <typename CharT>
static inline validation_errc ipv4_parse_number(const CharT* first, const CharT* last, uint32_t& number) {
    // If input is the empty string, then return failure
    if (first == last)
        return validation_errc::ipv4_non_numeric_part;

    // Figure out the base
    uint32_t radix = 10;
    if (first[0] == '0') {
        const std::size_t len = last - first;
        if (len == 1) {
            number = 0;
            return validation_errc::ok;
        }
        // len >= 2
        if (first[1] == 'X' || first[1] == 'x') {
            radix = 16;
            first += 2;
        } else {
            radix = 8;
            first += 1;
        }
        // Skip leading zeros (*)
        while (first < last && first[0] == '0')
            ++first;
    }
    // if all characters '0' (*) OR
    // if input is the empty string, then return zero
    if (first == last) {
        number = 0;
        return validation_errc::ok;
    }

    // Check length - max 32-bit value is
    // HEX: FFFFFFFF    (8 digits)
    // DEC: 4294967295  (10 digits)
    // OCT: 37777777777 (11 digits)
    if (last - first > 11)
        return validation_errc::ipv4_out_of_range_part; // int overflow

    // Check chars are valid digits and convert its sequence to number.
    // Use the 64-bit to get a big number (no hex, decimal, or octal
    // number can overflow a 64-bit number in <= 16 characters).
    uint64_t num = 0;
    if (radix <= 10) {
        const auto chmax = static_cast<CharT>('0' - 1 + radix);
        for (auto it = first; it != last; ++it) {
            const auto ch = *it;
            if (ch > chmax || ch < '0')
                return validation_errc::ipv4_non_numeric_part;
            num = num * radix + (ch - '0');
        }
    } else {
        // radix == 16
        for (auto it = first; it != last; ++it) {
            // This cast is safe because chars are ASCII
            const auto uch = static_cast<unsigned char>(*it);
            if (!detail::is_hex_char(uch))
                return validation_errc::ipv4_non_numeric_part;
            num = num * radix + detail::HexCharToValue(uch);
        }
    }

    // Check for 32-bit overflow.
    if (num > std::numeric_limits<uint32_t>::max())
        return validation_errc::ipv4_out_of_range_part; // int overflow

    number = static_cast<uint32_t>(num);
    return validation_errc::ok;
}

// IPv4 parser
// https://url.spec.whatwg.org/#concept-ipv4-parser
//
// - on success sets ipv4 value and returns validation_errc::ok
// - on failure returns validation error code
//
template <typename CharT>
inline validation_errc ipv4_parse(const CharT* first, const CharT* last, uint32_t& ipv4) {
    using UCharT = typename std::make_unsigned<CharT>::type;

    // 2. If the last item in parts is the empty string, then
    //    1. IPv4-empty-part validation error. (TODO-WARN)
    //
    // Failure comes from: 5.2 & "IPv4 number parser":
    // 1. If input is the empty string, then return failure.
    if (first == last)
        return validation_errc::ipv4_non_numeric_part;

    // <1>.<2>.<3>.<4>.<->
    const CharT* part[6];
    int dot_count = 0;

    // split on "."
    part[0] = first;
    for (auto it = first; it != last; ++it) {
        const auto uc = static_cast<UCharT>(*it);
        if (uc == '.') {
            if (dot_count == 4)
                // 3. If parts’s size is greater than 4, IPv4-too-many-parts validation error, return failure
                return validation_errc::ipv4_too_many_parts;
            if (part[dot_count] == it)
                // 5.2 & "IPv4 number parser":
                // 1. If input is the empty string, then return failure.
                return validation_errc::ipv4_non_numeric_part;
            part[++dot_count] = it + 1; // skip '.'
        } else if (!detail::is_ipv4_char(uc)) {
            // non IPv4 character
            return validation_errc::ipv4_non_numeric_part;
        }
    }

    // 2. If the last item in parts is the empty string, then:
    //    1. IPv4-empty-part validation error. (TODO-WARN)
    //    2. If parts’s size is greater than 1, then remove the last item from parts.
    int part_count = dot_count + 1;
    if (dot_count > 0 && part[dot_count] == last) {
        --part_count;
    } else {
        // the part[part_count] - 1 must point to the end of last part:
        part[part_count] = last + 1;
    }
    // 3. If parts’s size is greater than 4, IPv4-too-many-parts validation error, return failure
    if (part_count > 4)
        return validation_errc::ipv4_too_many_parts;

    // IPv4 numbers
    uint32_t number[4];
    for (int ind = 0; ind < part_count; ++ind) {
        const auto res = ipv4_parse_number(part[ind], part[ind + 1] - 1, number[ind]);
        // 5.2. If result is failure, IPv4-non-numeric-part validation error, return failure.
        if (res != validation_errc::ok) return res;
        // TODO-WARN: 5.3. If result[1] is true, IPv4-non-decimal-part validation error.
    }
    // TODO-WARN:
    // 6. If any item in numbers is greater than 255, IPv4-out-of-range-part validation error.

    // 7. If any but the last item in numbers is greater than 255, then return failure.
    for (int ind = 0; ind < part_count - 1; ++ind) {
        if (number[ind] > 255) return validation_errc::ipv4_out_of_range_part;
    }
    // 8. If the last item in numbers is greater than or equal to 256(5 − numbers’s size),
    // then return failure.
    ipv4 = number[part_count - 1];
    if (ipv4 > (std::numeric_limits<uint32_t>::max() >> (8 * (part_count - 1))))
        return validation_errc::ipv4_out_of_range_part;

    // 14.1. Increment ipv4 by n * 256**(3 - counter).
    for (int counter = 0; counter < part_count - 1; ++counter) {
        ipv4 += number[counter] << (8 * (3 - counter));
    }

    return validation_errc::ok;
}

// IPv4 serializer
// https://url.spec.whatwg.org/#concept-ipv4-serializer

void ipv4_serialize(uint32_t ipv4, std::string& output);


// IPv6

template <typename IntT, typename CharT>
inline IntT get_hex_number(const CharT*& pointer, const CharT* last) {
    IntT value = 0;
    while (pointer != last && detail::is_hex_char(*pointer)) {
        const auto uc = static_cast<unsigned char>(*pointer);
        value = value * 0x10 + detail::HexCharToValue(uc);
        ++pointer;
    }
    return value;
}

// IPv6 parser
// https://url.spec.whatwg.org/#concept-ipv6-parser
//
// - on success sets address value and returns true
// - on failure returns false
//
template <typename CharT>
inline validation_errc ipv6_parse(const CharT* first, const CharT* last, uint16_t(&address)[8]) {
    std::fill(std::begin(address), std::end(address), static_cast<uint16_t>(0));
    int piece_index = 0;    // zero
    int compress = 0;       // null
    bool is_ipv4 = false;

    const std::size_t len = last - first;
    // the shortest valid IPv6 address is "::"
    if (len < 2) {
        if (len == 0)
            return validation_errc::ipv6_too_few_pieces; // (8)
        switch (first[0]) {
        case ':':
            return validation_errc::ipv6_invalid_compression; // (5-1)
        case '.':
            return validation_errc::ipv4_in_ipv6_invalid_code_point; // (6-5-1)
        default:
            return detail::is_hex_char(first[0])
                ? validation_errc::ipv6_too_few_pieces // (8)
                : validation_errc::ipv6_invalid_code_point; // (6-7)
        }
    }

    const CharT* pointer = first;
    // 5. If c is U+003A (:), then:
    if (pointer[0] == ':') {
        if (pointer[1] != ':') {
            // 5.1. If remaining does not start with U+003A (:), IPv6-invalid-compression
            // validation error, return failure.
            return validation_errc::ipv6_invalid_compression;
        }
        pointer += 2;
        compress = ++piece_index;
    }

    // Main
    while (pointer < last) {
        if (piece_index == 8) {
            // 6.1. If pieceIndex is 8, IPv6-too-many-pieces validation error, return failure.
            return validation_errc::ipv6_too_many_pieces;
        }
        if (pointer[0] == ':') {
            if (compress) {
                // 6.2.1. If compress is non-null, IPv6-multiple-compression validation error,
                // return failure.
                return validation_errc::ipv6_multiple_compression;
            }
            ++pointer;
            compress = ++piece_index;
            continue;
        }

        // HEX
        auto pointer0 = pointer;
        const auto value = get_hex_number<uint16_t>(pointer, (last - pointer <= 4 ? last : pointer + 4));
        if (pointer != last) {
            const CharT ch = *pointer;
            if (ch == '.') {
                if (pointer == pointer0) {
                    // 6.5.1. If length is 0, IPv4-in-IPv6-invalid-code-point
                    // validation error, return failure.
                    return validation_errc::ipv4_in_ipv6_invalid_code_point;
                }
                pointer = pointer0;
                is_ipv4 = true;
                break;
            }
            if (ch == ':') {
                if (++pointer == last) {
                    // 6.6.2. If c is the EOF code point, IPv6-invalid-code-point
                    // validation error, return failure.
                    return validation_errc::ipv6_invalid_code_point;
                }
            } else {
                // 6.7. Otherwise, if c is not the EOF code point, IPv6-invalid-code-point
                // validation error, return failure.
                return validation_errc::ipv6_invalid_code_point;
            }
        }
        address[piece_index++] = value;
    }

    if (is_ipv4) {
        if (piece_index > 6) {
            // 6.5.3. If pieceIndex is greater than 6, IPv4-in-IPv6-too-many-pieces
            // validation error, return failure.
            return validation_errc::ipv4_in_ipv6_too_many_pieces;
        }
        int numbers_seen = 0;
        while (pointer < last) {
            if (numbers_seen > 0) {
                if (*pointer == '.' && numbers_seen < 4) {
                    ++pointer;
                } else {
                    // 6.5.5.2.2. Otherwise, IPv4-in-IPv6-invalid-code-point
                    // validation error, return failure.
                    return validation_errc::ipv4_in_ipv6_invalid_code_point;
                }
            }
            if (pointer == last || !detail::is_ascii_digit(*pointer)) {
                // 6.5.5.3. If c is not an ASCII digit, IPv4-in-IPv6-invalid-code-point
                // validation error, return failure.
                return validation_errc::ipv4_in_ipv6_invalid_code_point;
            }
            // While c is an ASCII digit, run these subsubsteps
            unsigned ipv4Piece = *(pointer++) - '0';
            while (pointer != last && detail::is_ascii_digit(*pointer)) {
                // 6.5.5.4.2. Otherwise, if ipv4Piece is 0, IPv4-in-IPv6-invalid-code-point
                // validation error, return failure.
                if (ipv4Piece == 0) // leading zero
                    return validation_errc::ipv4_in_ipv6_invalid_code_point;
                ipv4Piece = ipv4Piece * 10 + (*pointer - '0');
                // 6.5.5.4.3. If ipv4Piece is greater than 255, IPv4-in-IPv6-out-of-range-part
                // validation error, return failure.
                if (ipv4Piece > 255)
                    return validation_errc::ipv4_in_ipv6_out_of_range_part;
                ++pointer;
            }
            address[piece_index] = static_cast<uint16_t>(address[piece_index] * 0x100 + ipv4Piece);
            ++numbers_seen;
            if (!(numbers_seen & 1)) // 2 or 4
                ++piece_index;
        }
        // 6.5.6. If numbersSeen is not 4, IPv4-in-IPv6-too-few-parts
        // validation error, return failure.
        if (numbers_seen != 4)
            return validation_errc::ipv4_in_ipv6_too_few_parts;
    }

    // Finale
    if (compress) {
        if (const int diff = 8 - piece_index) {
            for (int ind = piece_index - 1; ind >= compress; --ind) {
                address[ind + diff] = address[ind];
                address[ind] = 0;
            }
        }
    } else if (piece_index != 8) {
        // Otherwise, if compress is null and pieceIndex is not 8, IPv6-too-few-pieces
        // validation error, return failure.
        return validation_errc::ipv6_too_few_pieces;
    }
    return validation_errc::ok;
}

// IPv6 serializer
// https://url.spec.whatwg.org/#concept-ipv6-serializer

void ipv6_serialize(const uint16_t(&address)[8], std::string& output);


} // namespace upa

#endif // UPA_URL_IP_H
