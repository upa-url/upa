#ifndef TEST_UTILS__H
#define TEST_UTILS__H

#include "config.h"
#include <algorithm>
#include <initializer_list>
#include <utility> // std::pair
#include <string>
#ifdef __cpp_char8_t
# include <string_view>
#endif


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
    return std::string(whatwg::make_string(std::forward<T>(val)));
}

// for url_search_params

template <class T>
inline bool param_eq(const std::string* pval, const T& value) {
    return pval != nullptr && *pval == value;
}

template <class List, class T>
inline bool list_eq(const List& val, std::initializer_list<T> lst) {
#ifdef WHATWG__CPP_14
    return std::equal(std::begin(val), std::end(val), std::begin(lst), std::end(lst));
#else
    return
        val.size() == lst.size() &&
        std::equal(std::begin(val), std::end(val), std::begin(lst));
#endif
}

template <class T>
using pairs_list_t = std::initializer_list<std::pair<T, T>>;;


#endif // TEST_UTILS__H
