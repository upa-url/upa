// Copyright 2016-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_UTIL_H
#define UPA_UTIL_H

#include "config.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#ifdef __cpp_lib_start_lifetime_as
# include <memory>
#endif
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace upa::util {

// For use in static_assert, workaround before CWG2518/P2593R1

template<class>
constexpr bool false_v = false;

// Integers

// Some functions here use unsigned arithmetics with unsigned overflow intentionally.
// So unsigned-integer-overflow checks are disabled for these functions in the Clang
// UndefinedBehaviorSanitizer (UBSan) with
// __attribute__((no_sanitize("unsigned-integer-overflow"))).

// Utility class to get unsigned (abs) max, min values of (signed) integer type
template <typename T, typename UT = std::make_unsigned_t<T>>
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
    typename UT = std::make_unsigned_t<T>,
    std::enable_if_t<std::is_integral_v<T>, int> = 0>
#if defined(__clang__)
__attribute__((no_sanitize("unsigned-integer-overflow")))
#endif
constexpr Out checked_diff(T a, T b) {
    if (a >= b) {
        const UT diff = static_cast<UT>(static_cast<UT>(a) - static_cast<UT>(b));
        if (diff <= unsigned_limit<Out>::max())
            return static_cast<Out>(diff);
    } else if constexpr (std::is_signed_v<Out>) {
        // b > a ==> diff >= 1
        const UT diff = static_cast<UT>(static_cast<UT>(b) - static_cast<UT>(a));
        if (diff <= unsigned_limit<Out>::min())
            return static_cast<Out>(0) - static_cast<Out>(diff - 1) - 1;
    }
    throw std::length_error("too big difference");
}

// Cast integer value to corresponding unsigned type

template <typename T, typename UT = std::make_unsigned_t<T>>
constexpr auto to_unsigned(T n) noexcept -> UT {
    return static_cast<UT>(n);
}

// Append unsigned integer to string

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

// Convert any char type string to std::basic_string_view

template <typename T, typename SizeT>
constexpr std::basic_string_view<T> to_string_view(const T* str, SizeT length) {
    assert(length >= 0);
    return { str, static_cast<std::size_t>(length) };
}

template <typename T, typename CharT, typename SizeT,
    std::enable_if_t<!std::is_same_v<T, CharT> && sizeof(T) == sizeof(CharT), int> = 0>
inline std::basic_string_view<T> to_string_view(const CharT* src, SizeT length) {
    assert(length >= 0);
    const auto size = static_cast<std::size_t>(length);
#ifdef __cpp_lib_start_lifetime_as
    const T* str = std::start_lifetime_as_array<T>(src, size);
#else
    UPA_ALIASING_BARRIER(src)
    const T* str = reinterpret_cast<const T*>(src);
#endif
    return { str, size };
}

// Append data to string

constexpr std::size_t add_sizes(std::size_t size1, std::size_t size2, std::size_t max_size) {
    if (max_size - size1 < size2)
        throw std::length_error("too big size");
    // now it is safe to add sizes
    return size1 + size2;
}

template <class CharT, class StrT>
inline void append(std::basic_string<CharT>& dest, const StrT& src) {
#ifdef _MSC_VER
    if constexpr (!std::is_same_v<typename StrT::value_type, CharT>) {
        // the value_type of dest and src are different
        dest.reserve(add_sizes(dest.size(), src.size(), dest.max_size()));
        for (const auto c : src)
            dest.push_back(static_cast<CharT>(c));
    } else
#endif
    dest.append(src.begin(), src.end());
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
#else
    dest.resize(new_size);
    std::transform(first, last, dest.data() + old_size, unary_op);
#endif
}

template <typename CharT>
constexpr char ascii_to_lower_char(CharT c) noexcept {
    return static_cast<char>((c <= 'Z' && c >= 'A') ? (c | 0x20) : c);
}

template <class CharT>
inline void append_ascii_lowercase(std::string& dest, const CharT* first, const CharT* last) {
    util::append_tr(dest, first, last, ascii_to_lower_char<CharT>);
}

// Finders

template <class InputIt>
inline bool contains_null(InputIt first, InputIt last) {
    return std::find(first, last, '\0') != last;
}

template <class CharT>
constexpr bool has_xn_label(const CharT* first, const CharT* last) {
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


} // namespace upa::util

#endif // UPA_UTIL_H
