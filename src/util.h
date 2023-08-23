// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_UTIL_H
#define UPA_UTIL_H

#include "config.h"
#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace upa { // NOLINT(modernize-concat-nested-namespaces)
namespace util {

// Integers

// Some functions here use unsigned arithmetics with unsigned overflow intentionally.
// So unsigned-integer-overflow checks are disabled for these functions in the Clang
// UndefinedBehaviorSanitizer (UBSan) with
// __attribute__((no_sanitize("unsigned-integer-overflow"))).

// Utility class to get unsigned (abs) max, min values of (signed) integer type
template <typename T, typename UT = typename std::make_unsigned<T>::type>
struct unsigned_limit {
    static constexpr UT max() noexcept {
        return static_cast<UT>(std::numeric_limits<T>::max());
    }

#if defined(__clang__)
    __attribute__((no_sanitize("unsigned-integer-overflow")))
#endif
    static constexpr UT min() noexcept {
        // http://en.cppreference.com/w/cpp/language/implicit_conversion
        // Integral conversions: If the destination type is unsigned, the resulting
        // value is the smallest unsigned value equal to the source value modulo 2^n
        // where n is the number of bits used to represent the destination type.
        // https://en.wikipedia.org/wiki/Modular_arithmetic#Congruence
        return static_cast<UT>(0) - static_cast<UT>(std::numeric_limits<T>::min());
    }
};

// Returns difference between a and b (a - b), if result is not representable
// by the type Out - throws exception.
template <typename Out, typename T,
    typename UT = typename std::make_unsigned<T>::type,
    typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
#if defined(__clang__)
__attribute__((no_sanitize("unsigned-integer-overflow")))
#endif
inline Out checked_diff(T a, T b) {
    if (a >= b) {
        const UT diff = static_cast<UT>(static_cast<UT>(a) - static_cast<UT>(b));
        if (diff <= unsigned_limit<Out>::max())
            return static_cast<Out>(diff);
    } else if (std::is_signed<Out>::value) {
        // b > a ==> diff >= 1
        const UT diff = static_cast<UT>(static_cast<UT>(b) - static_cast<UT>(a));
        if (diff <= unsigned_limit<Out>::min())
            return static_cast<Out>(0) - static_cast<Out>(diff - 1) - 1;
    }
    throw std::length_error("too big difference");
}

// Cast integer value to corresponding unsigned type

template <typename T, typename UT = typename std::make_unsigned<T>::type>
constexpr auto to_unsigned(T n) noexcept -> UT {
    return static_cast<UT>(n);
}

// Append data to string

inline std::size_t add_sizes(std::size_t size1, std::size_t size2, std::size_t max_size) {
    if (max_size - size1 < size2)
        throw std::length_error("too big size");
    // now it is safe to add sizes
    return size1 + size2;
}

template <class StrT>
inline void append(std::string& dest, const StrT& src) {
#ifdef _MSC_VER
    dest.reserve(add_sizes(dest.size(), src.size(), dest.max_size()));
    for (const auto c : src)
        dest.push_back(static_cast<char>(c));
#else
    dest.append(src.begin(), src.end());
#endif
}

template <class CharT, class UnaryOperation>
inline void append_tr(std::string& dest, const CharT* first, const CharT* last, UnaryOperation unary_op) {
    const std::size_t old_size = dest.size();
    const std::size_t src_size = last - first;
    const std::size_t new_size = add_sizes(old_size, src_size, dest.max_size());

#ifdef __cpp_lib_string_resize_and_overwrite
    dest.resize_and_overwrite(new_size, [&](char* buff, std::size_t) {
        std::transform(first, last, buff + old_size, unary_op);
        return new_size;
    });
#elif defined(UPA_CPP_17)
    dest.resize(new_size);
    std::transform(first, last, dest.data() + old_size, unary_op);
#else
    dest.reserve(new_size);
    std::transform(first, last, std::back_inserter(dest), unary_op);
#endif
}

template <typename CharT>
constexpr char ascii_to_lower_char(CharT c) {
    return static_cast<char>((c <= 'Z' && c >= 'A') ? (c | 0x20) : c);
}

template <class CharT>
inline void append_ascii_lowercase(std::string& dest, const CharT* first, const CharT* last) {
    util::append_tr(dest, first, last, ascii_to_lower_char<CharT>);
}

// Finders

template <class CharT>
UPA_CONSTEXPR_17 bool has_xn_label(const CharT* first, const CharT* last) {
    if (last - first >= 4) {
        // search for labels starting with "xn--"
        const auto end = last - 4;
        for (auto p = first; ; ++p) { // skip '.'
            // "XN--", "xn--", ...
            if ((p[0] | 0x20) == 'x' && (p[1] | 0x20) == 'n' && p[2] == '-' && p[3] == '-')
                return true;
            p = std::char_traits<CharT>::find(p, end - p, '.');
            if (p == nullptr) break;
        }
    }
    return false;
}


} // namespace util
} // namespace upa

#endif // UPA_UTIL_H
