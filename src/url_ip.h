// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef WHATWG_URL_IP_H
#define WHATWG_URL_IP_H

#include "url_percent_encode.h"
#include "url_result.h"
#include <cassert>
#include <cstdint> // uint32_t
#include <limits>
#include <string>

namespace whatwg {

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

// IPv4 number parser
// https://url.spec.whatwg.org/#ipv4-number-parser
//
// - on success sets number value and returns url_result::Ok
// - if resulting number can not be represented by uint32_t value, then returns
//   url_result::InvalidIpv4Address 
// - on failure (non digit char) returns url_result::False
//
// TODO-WARN: validationError

template <typename CharT>
static inline url_result ipv4_parse_number(const CharT* first, const CharT* last, uint32_t& number) {
    // Figure out the base
    uint32_t radix;
    if (first < last && first[0] == '0') {
        const std::size_t len = last - first;
        if (len == 1) {
            number = 0;
            return url_result::Ok;
        } else {
            // len >= 2
            if (first[1] == 'X' || first[1] == 'x') {
                radix = 16;
                first += 2;
            } else {
                radix = 8;
                first += 1;
            }
        }
        // Skip leading zeros (*)
        while (first < last && first[0] == '0')
            ++first;
    } else {
        radix = 10;
    }
    // if all characters '0' (*) OR
    // if input is the empty string, then return zero
    if (first == last) {
        number = 0;
        return url_result::Ok;
    }

    // check if valid digits (we known all chars are ASCII)
    if (radix <= 10) {
        const CharT chmax = static_cast<CharT>('0' - 1 + radix);
        for (auto it = first; it != last; ++it) {
            if (*it > chmax || *it < '0')
                return url_result::False;
        }
    } else {
        for (auto it = first; it != last; ++it) {
            if (!detail::isHexChar(static_cast<unsigned char>(*it)))
                return url_result::False;
        }
    }

    // All characters are valid digits, convert them to integer:
    // max 32-bit value is
    // HEX: FFFFFFFF    (8 digits)
    // DEC: 4294967295  (10 digits)
    // OCT: 37777777777 (11 digits)
    if (last - first > 11)
        return url_result::InvalidIpv4Address; // int overflow

    // Use the 64-bit to get a big number (no hex, decimal, or octal
    // number can overflow a 64-bit number in <= 16 characters).
    uint64_t num = 0;
    if (radix <= 10) {
        for (auto it = first; it != last; ++it)
            num = num * radix + (*it - '0');
    } else {
        for (auto it = first; it != last; ++it)
            num = num * radix + detail::HexCharToValue(static_cast<unsigned char>(*it));
    }

    // Check for 32-bit overflow.
    if (num > std::numeric_limits<uint32_t>::max())
        return url_result::InvalidIpv4Address; // int overflow

    number = static_cast<uint32_t>(num);
    return url_result::Ok;
}

// IPv4 parser
// https://url.spec.whatwg.org/#concept-ipv4-parser
//
// - on success sets ipv4 value and returns url_result::Ok
// - on failure (when all parts are valid numbers, but any of them is too big),
//   returns url_result::InvalidIpv4Address
// - if input is not IPv4 adrress (any part is empty or contains non digit),
//   then returns url_result::False (standard says "return input").

template <typename CharT>
inline url_result ipv4_parse(const CharT* first, const CharT* last, uint32_t& ipv4) {
    using UCharT = typename std::make_unsigned<CharT>::type;

    // Comes from: 6.1. If part is the empty string, then return input.
    if (first == last)
        return url_result::False;

    // <1>.<2>.<3>.<4>.<->
    const CharT* part[6];
    int dot_count = 0;

    // split on "."
    part[0] = first;
    for (auto it = first; it != last; ++it) {
        UCharT uc = static_cast<UCharT>(*it);
        if (uc == '.') {
            if (dot_count == 4)
                return url_result::False; // 4. If parts’s size is greater than 4, then return input.
            if (part[dot_count] == it)
                return url_result::False; // 6.1. If part is the empty string, then return input.
            part[++dot_count] = it + 1; // skip '.'
        } else if (!detail::isIPv4Char(uc)) {
            // not IPv4 character
            return url_result::False;
        }
    }

    // 3. If the last item in parts is the empty string, then:
    //    1. Set validationError to true.
    //    2. If parts’s size is greater than 1, then remove the last item from parts.
    int part_count;
    if (dot_count > 0 && part[dot_count] == last) {
        part_count = dot_count;
    } else {
        part_count = dot_count + 1;
        part[part_count] = last + 1; // bus -1
    }
    // 4. If parts’s size is greater than 4, then return input.
    if (part_count > 4)
        return url_result::False;

    // IPv4 numbers
    uint32_t number[4];
    for (int ind = 0; ind < part_count; ++ind) {
        url_result res = ipv4_parse_number(part[ind], part[ind + 1] - 1, number[ind]);
        if (res != url_result::Ok) return res;
    }
    // TODO-WARN:
    // 7. If validationError is true, validation error.
    // 8. If any item in numbers is greater than 255, validation error.

    // 9. If any but the last item in numbers is greater than 255, then return failure.
    for (int ind = 0; ind < part_count - 1; ++ind) {
        if (number[ind] > 255) return url_result::InvalidIpv4Address;
    }
    // 10. If the last item in numbers is greater than or equal to 256**(5 - numbers’s size),
    // validation error, return failure.
    ipv4 = number[part_count - 1];
    if (ipv4 > (std::numeric_limits<uint32_t>::max() >> (8 * (part_count - 1))))
        return url_result::InvalidIpv4Address;

    // 14.1. Increment ipv4 by n * 256**(3 - counter).
    for (int counter = 0; counter < part_count - 1; ++counter) {
        ipv4 += number[counter] << (8 * (3 - counter));
    }

    return url_result::Ok;
}

// IPv4 serializer
// https://url.spec.whatwg.org/#concept-ipv4-serializer

void ipv4_serialize(uint32_t ipv4, std::string& output);


// IPv6

template <typename CharT, typename IntT>
inline void get_hex_number(const CharT*& pointer, const CharT* last, IntT& value) {
    value = 0;
    while (pointer != last && detail::isHexChar(*pointer)) {
        const unsigned char uc = static_cast<unsigned char>(*pointer);
        value = value * 0x10 + detail::HexCharToValue(uc);
        ++pointer;
    }
}

// IPv6 parser
// https://url.spec.whatwg.org/#concept-ipv6-parser
//
// - on success sets address value and returns true
// - on failure returns false
//

template <typename CharT>
inline bool ipv6_parse(const CharT* first, const CharT* last, uint16_t(&address)[8]) {
    std::fill(std::begin(address), std::end(address), uint16_t(0));
    int piece_index = 0;    // zero
    int compress = 0;       // null
    bool is_ipv4 = false;

    const std::size_t len = last - first;
    // minimalus yra "::"
    if (len < 2) return false;

    const CharT* pointer = first;
    // 5. If c is ":", run these substeps:
    if (pointer[0] == ':') {
        if (pointer[1] != ':') {
            // TODO-ERR: validation error
            return false;
        }
        pointer += 2;
        compress = ++piece_index;
    }

    // Main
    while (pointer < last) {
        if (piece_index == 8) {
            // TODO-ERR: validation error
            return false;
        }
        if (pointer[0] == ':') {
            if (compress) {
                // TODO-ERR: validation error
                return false;
            }
            ++pointer;
            compress = ++piece_index;
            continue;
        }

        // HEX
        uint16_t value;
        auto pointer0 = pointer;
        get_hex_number(pointer, (last - pointer <= 4 ? last : pointer + 4), value);
        if (pointer != last) {
            const CharT ch = *pointer;
            if (ch == '.') {
                if (pointer == pointer0) {
                    // TODO-ERR: validation error
                    return false;
                }
                pointer = pointer0;
                is_ipv4 = true;
                break;
            } if (ch == ':') {
                if (++pointer == last) {
                    // TODO-ERR: validation error
                    return false;
                }
            } else {
                // TODO-ERR: validation error
                return false;
            }
        }
        address[piece_index++] = value;
    }

    if (is_ipv4) {
        if (piece_index > 6) {
            // TODO-ERR: validation error
            return false;
        }
        int numbers_seen = 0;
        while (pointer < last) {
            if (numbers_seen > 0) {
                if (*pointer == '.' && numbers_seen < 4) {
                    ++pointer;
                } else {
                    // TODO-ERR: validation error
                    return false;
                }
            }
            if (pointer == last || !detail::is_ascii_digit(*pointer)) {
                // TODO-ERR: validation error
                return false;
            }
            // While c is an ASCII digit, run these subsubsteps
            unsigned ipv4Piece = *(pointer++) - '0';
            while (pointer != last && detail::is_ascii_digit(*pointer)) {
                if (ipv4Piece == 0) // leading zero
                    return false; // TODO-ERR: validation error
                ipv4Piece = ipv4Piece * 10 + (*pointer - '0');
                if (ipv4Piece > 255)
                    return false; // TODO-ERR: validation error
                ++pointer;
            }
            address[piece_index] = static_cast<uint16_t>(address[piece_index] * 0x100 + ipv4Piece);
            ++numbers_seen;
            if (!(numbers_seen & 1)) // 2 or 4
                ++piece_index;
        }
        // If c is the EOF code point and numbersSeen is not 4
        if (numbers_seen != 4)  {
            //TODO-ERR: validation error
            return false;
        }
    }

    // Finale
    if (compress) {
        if (int diff = 8 - piece_index) {
            for (int ind = piece_index - 1; ind >= compress; --ind) {
                address[ind + diff] = address[ind];
                address[ind] = 0;
            }
        }
    } else if (piece_index != 8) {
        // TODO-ERR: validation error
        return false;
    }
    return true;
}

// IPv6 serializer
// https://url.spec.whatwg.org/#concept-ipv6-serializer

void ipv6_serialize(const uint16_t(&address)[8], std::string& output);


} // namespace whatwg

#endif // WHATWG_URL_IP_H
