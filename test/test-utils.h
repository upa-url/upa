// Copyright 2016-2026 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_TEST_UTILS_H
#define UPA_TEST_UTILS_H

#include "upa/config.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iomanip>
#include <sstream>
#include <string>
#ifdef __cpp_char8_t
# include <string_view>
#endif
#include <utility> // std::pair


#ifdef __cpp_char8_t
UPA_CONSTEXPR_20 bool operator==(std::string_view lhs, std::u8string_view rhs) noexcept {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
        [](char a, char8_t b) { return a == char(b) && char8_t(a) == b; }
    );
}
#endif

// to convert char8_t* literal to std::string
template <class T>
inline std::string mk_string(T&& val) {
    return std::string(upa::make_string(std::forward<T>(val)));
}

// encodeURIComponent(...) implementation as described in ECMAScript Language Specification (ECMA-262):
// https://tc39.es/ecma262/#sec-encodeuricomponent-uricomponent
// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURIComponent
// Input must be valid UTF-8.

std::string encodeURIComponent(const std::string& str) {
    std::ostringstream strm;
    for (char c : str) {
        // Not escapes: A-Z a-z 0-9 - _ . ! ~ * ' ( )
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '!' || c == '~' || c == '*' || c == '\'' || c == '(' || c == ')') {
            strm << c;
        } else {
            // percent encode
            const auto uc = static_cast<unsigned char>(c);
            strm << '%' << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << int(uc);
        }
    }
    return strm.str();
}

bool encode_utf8(std::string& out, std::uint32_t cp) {
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
        return false;

    char bytes[4];
    std::size_t length;

    if (cp <= 0x7F) {
        bytes[0] = static_cast<char>(cp);
        length = 1;
    } else if (cp <= 0x7FF) {
        bytes[0] = static_cast<char>(0xC0 | (cp >> 6));
        bytes[1] = static_cast<char>(0x80 | (cp & 0x3F));
        length = 2;
    } else if (cp <= 0xFFFF) {
        bytes[0] = static_cast<char>(0xE0 | (cp >> 12));
        bytes[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        bytes[2] = static_cast<char>(0x80 | (cp & 0x3F));
        length = 3;
    } else {
        bytes[0] = static_cast<char>(0xF0 | (cp >> 18));
        bytes[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        bytes[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        bytes[3] = static_cast<char>(0x80 | (cp & 0x3F));
        length = 4;
    }

    out.append(bytes, length);
    return true;
}

// for url_search_params

template <class T>
inline bool param_eq(const std::string* pval, const T& value) {
    return pval != nullptr && *pval == value;
}

template <class List, class T>
inline bool list_eq(const List& val, std::initializer_list<T> lst) {
    return std::equal(std::begin(val), std::end(val), std::begin(lst), std::end(lst));
}

template <class T>
using pairs_list_t = std::initializer_list<std::pair<T, T>>;;


#endif // UPA_TEST_UTILS_H
