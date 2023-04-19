// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef WHATWG_UTIL_H
#define WHATWG_UTIL_H

#include "config.h"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace whatwg { // NOLINT(modernize-concat-nested-namespaces)
namespace util {


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
#elif defined(WHATWG_CPP_17) 
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


} // namespace util
} // namespace whatwg

#endif // WHATWG_UTIL_H
