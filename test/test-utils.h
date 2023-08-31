// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_TEST_UTILS_H
#define UPA_TEST_UTILS_H

#include "upa/config.h"
#include <algorithm>
#include <initializer_list>
#include <iomanip>
#include <sstream>
#include <string>
#ifdef __cpp_char8_t
# include <string_view>
#endif
#include <utility> // std::pair


#ifdef __cpp_char8_t
inline /* constexpr */ bool operator==(std::string_view lhs, std::u8string_view rhs) noexcept {
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
    std::stringstream strm;
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

// for url_search_params

template <class T>
inline bool param_eq(const std::string* pval, const T& value) {
    return pval != nullptr && *pval == value;
}

template <class List, class T>
inline bool list_eq(const List& val, std::initializer_list<T> lst) {
#ifdef UPA_CPP_14
    return std::equal(std::begin(val), std::end(val), std::begin(lst), std::end(lst));
#else
    return
        val.size() == lst.size() &&
        std::equal(std::begin(val), std::end(val), std::begin(lst));
#endif
}

template <class T>
using pairs_list_t = std::initializer_list<std::pair<T, T>>;;


#endif // UPA_TEST_UTILS_H
