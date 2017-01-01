#ifndef WHATWG_URL_IP_H
#define WHATWG_URL_IP_H

#include "url_canon.h"
#include <cassert>
#include <cstdint> // uint32_t
#include <limits>
#include <string>

namespace whatwg {

enum ParseResult {
    RES_OK = 0,
    RES_FALSE,
    RES_ERROR,
};

// IPv4 parser

// TODO-WARN: syntaxViolationFlag
template <typename CharT>
static inline ParseResult ipv4_parse_number(const CharT* first, const CharT* last, uint32_t& number) {
    assert(first < last);

    // Figure out the base
    detail::SharedCharTypes base;
    uint32_t radix;
    if (first[0] == '0') {
        const size_t len = last - first;
        if (len == 1) {
            number = 0;
            return RES_OK;
        } else {
            // len >= 2
            if (first[1] == 'X' || first[1] == 'x') {
                base = detail::CHAR_HEX;
                radix = 16;
                first += 2;
            } else {
                base = detail::CHAR_OCT;
                radix = 8;
                first += 1;
            }
        }
        // Skip leading zeros
        while (first < last && first[0] == '0')
            first++;
        // if input is the empty string, then return zero
        if (first == last) {
            number = 0;
            return RES_OK;
        }
    } else {
        base = detail::CHAR_DEC;
        radix = 10;
    }

    // check if valid digits (we known all chars are ASCII)
    for (auto it = first; it != last; it++) {
        const unsigned char uc = static_cast<unsigned char>(*it);
        if (!IsCharOfType(uc, base))
            return RES_FALSE;
    }

    // convert to integer
    // max 32-bit value is
    // HEX: FFFFFFFF    (8 digits)
    // DEC: 4294967295  (10 digits)
    // OCT: 37777777777 (11 digits)
    if (last - first > 11)
        return RES_ERROR; // overflow

    // Use the 64-bit to get a big number (no hex, decimal, or octal
    // number can overflow a 64-bit number in <= 16 characters).
    uint64_t num = 0;
    if (radix <= 10) {
        for (auto it = first; it != last; it++)
            num = num * radix + (*it - '0');
    } else {
        for (auto it = first; it != last; it++)
            num = num * radix + detail::HexCharToValue(static_cast<unsigned char>(*it));
    }

    // Check for 32-bit overflow.
    if (num > std::numeric_limits<uint32_t>::max())
        return RES_ERROR; // overflow

    number = static_cast<uint32_t>(num);
    return RES_OK;
}

template <typename CharT>
inline ParseResult ipv4_parse(const CharT* first, const CharT* last, uint32_t& ipv4) {
    typedef std::make_unsigned<CharT>::type UCharT;

    // <1>.<2>.<3>.<4>.<->
    const CharT* part[6];
    int dot_count = 0;

    // split on "."
    part[0] = first;
    for (auto it = first; it != last; it++) {
        UCharT uc = static_cast<UCharT>(*it);
        if (uc == '.') {
            if (dot_count == 4)
                return RES_FALSE; // 4. If parts has more than four items, return input
            if (part[dot_count] == it)
                return RES_FALSE; // 6.1. If part is the empty string, return input
            part[++dot_count] = it + 1; // skip '.'
        } else if (uc >= 0x80 || !detail::IsIPv4Char(static_cast<unsigned char>(uc))) {
            // not IPv4 character
            return RES_FALSE;
        }
    }

    // 3. If the last item in parts is the empty string, set syntaxViolationFlag and remove the last item from parts
    int part_count;
    if (dot_count > 0 && part[dot_count] == last) {
        part_count = dot_count;
    } else {
        part_count = dot_count + 1;
        part[part_count] = last + 1; // bus -1
    }
    // 4. If parts has more than four items, return input
    if (part_count > 4)
        return RES_FALSE;

    // IPv4 numbers
    uint32_t number[4];
    for (int ind = 0; ind < part_count; ind++) {
        ParseResult res = ipv4_parse_number(part[ind], part[ind + 1] - 1, number[ind]);
        if (res != RES_OK) return res;
    }
    // TODO-WARN:
    // 7. If syntaxViolationFlag is set, syntax violation
    // 8. If any item in numbers is greater than 255, syntax violation

    // 9. If any but the last item in numbers is greater than 255, return failure
    for (int ind = 0; ind < part_count - 1; ind++) {
        if (number[ind] > 255) return RES_ERROR;
    }
    // 10. If the last item in numbers is greater than or equal to 256**(5 - the number of items in numbers),
    // syntax violation, return failure
    ipv4 = number[part_count - 1];
    if (ipv4 > (std::numeric_limits<uint32_t>::max() >> (8 * (part_count - 1))))
        return RES_ERROR;

    // 14.1. Increment ipv4 by n * 256**(3 - counter). 
    for (int counter = 0; counter < part_count - 1; counter++) {
        ipv4 += number[counter] << (8 * (3 - counter));
    }

    return RES_OK;
}

// IPv4 serializer

void ipv4_serialize(uint32_t ipv4, std::string& output);


} // namespace whatwg

#endif // WHATWG_URL_IP_H
