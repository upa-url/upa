// Copyright 2016-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

/**************************************************************
// Usage example:

template <class StrT, enable_if_str_arg_t<StrT> = 0>
inline void procfn(StrT&& str) {
    const auto inp = make_str_arg(std::forward<StrT>(str));
    const auto* first = inp.begin();
    const auto* last = inp.end();
    // do something with first ... last
}

**************************************************************/
#ifndef UPA_STR_ARG_H
#define UPA_STR_ARG_H

#include "config.h"
#include "url_utf.h"
#include "util.h"
#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace upa {

// String view type

using string_view = std::string_view;

// Supported char and size types

template<class CharT>
constexpr bool is_char_type_v =
    std::is_same_v<CharT, char> ||
#ifdef __cpp_char8_t
    std::is_same_v<CharT, char8_t> ||
#endif
    std::is_same_v<CharT, char16_t> ||
    std::is_same_v<CharT, char32_t> ||
    std::is_same_v<CharT, wchar_t>;

template<class SizeT>
constexpr bool is_size_type_v =
    std::is_convertible_v<SizeT, std::size_t> ||
    std::is_convertible_v<SizeT, std::ptrdiff_t>;

// See: https://en.cppreference.com/w/cpp/concepts/derived_from
template<class Derived, class Base>
constexpr bool is_derived_from_v =
    std::is_base_of_v<Base, Derived> &&
    std::is_convertible_v<const volatile Derived*, const volatile Base*>;


// string args helper class

template <typename T>
class str_arg {
public:
    // output value type
    using value_type =
        // char, char8_t
        std::conditional_t<sizeof(T) == sizeof(char), char,
        // char16_t, char32_t, wchar_t
        std::conditional_t<sizeof(T) == sizeof(char16_t), char16_t,
        std::conditional_t<sizeof(T) == sizeof(char32_t), char32_t,
        T>>>;

    // constructors
    constexpr str_arg(const str_arg&) noexcept = default;

    template <typename CharT, typename SizeT,
        std::enable_if_t<sizeof(CharT) == sizeof(value_type), int> = 0,
        std::enable_if_t<is_size_type_v<SizeT>, int> = 0>
    constexpr str_arg(const CharT* s, SizeT length)
        : str_arg{ util::to_string_view<value_type>(s, length) }
    {}

    template <typename CharT,
        std::enable_if_t<sizeof(CharT) == sizeof(value_type), int> = 0>
    constexpr str_arg(const CharT* first, const CharT* last)
        : str_arg{ util::to_string_view<value_type>(first, last - first) }
    { assert(first <= last); }

    // destructor
    UPA_CONSTEXPR_20 ~str_arg() noexcept = default;

    // assignment is not used
    str_arg& operator=(const str_arg&) = delete;

    // output
    constexpr const value_type* begin() const noexcept {
        return first_;
    }
    constexpr const value_type* end() const noexcept {
        return last_;
    }
    constexpr const value_type* data() const noexcept {
        return first_;
    }
    constexpr std::size_t length() const noexcept {
        return last_ - first_;
    }
    constexpr std::size_t size() const noexcept {
        return length();
    }

private:
    constexpr str_arg(std::basic_string_view<value_type> sv)
        : first_{ sv.data() }
        , last_{ sv.data() + sv.size() }
    {}

    const value_type* first_;
    const value_type* last_;
};

// str_arg deduction guide
template <typename CharT, typename T>
str_arg(const CharT*, T) -> str_arg<CharT>;

// String type helpers

template<class T>
using remove_cvptr_t = std::remove_cv_t<std::remove_pointer_t<T>>;

template<class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

namespace detail {

// Check that StrT has data() and size() members of supported types

template<class StrT, typename = void>
constexpr bool has_data_and_size_v = false;

template<class StrT>
constexpr bool has_data_and_size_v<StrT, std::void_t<
    decltype(std::declval<StrT>().data()),
    decltype(std::declval<StrT>().size())>> =
    // check members types
    std::is_pointer_v<decltype(std::declval<StrT>().data())> &&
    is_char_type_v<remove_cvptr_t<decltype(std::declval<StrT>().data())>> &&
    is_size_type_v<decltype(std::declval<StrT>().size())>;

// Check StrT is convertible to std::basic_string_view

template<class StrT, typename CharT>
constexpr bool convertible_to_string_view_v =
    std::is_convertible_v<StrT, std::basic_string_view<CharT>> &&
    !std::is_same_v<StrT, std::nullptr_t>;

// Common class for converting input to str_arg

template<typename CharT, class ArgT>
struct str_arg_char_common {
    using type = CharT;
    static constexpr str_arg<CharT> to_str_arg(ArgT str) {
        return { str.data(), str.size() };
    }
};

// StrT has data() and size() members

template<class StrT, typename = void>
struct str_arg_char_data_size {};

template<class StrT>
struct str_arg_char_data_size<StrT, std::enable_if_t<
    has_data_and_size_v<StrT>>>
    : str_arg_char_common<
    remove_cvptr_t<decltype(std::declval<StrT>().data())>,
    remove_cvref_t<StrT> const&> {};

// Default str_arg_char implementation

template<class StrT, typename = void>
struct str_arg_char_default : str_arg_char_data_size<StrT> {};

// StrT is convertible to std::basic_string_view
template<class StrT>
struct str_arg_char_default<StrT, std::enable_if_t<
    convertible_to_string_view_v<StrT, char>>>
    : str_arg_char_common<char, std::basic_string_view<char>> {};

#ifdef __cpp_char8_t
template<class StrT>
struct str_arg_char_default<StrT, std::enable_if_t<
    convertible_to_string_view_v<StrT, char8_t> &&
    !convertible_to_string_view_v<StrT, char>>>
    : str_arg_char_common<char8_t, std::basic_string_view<char8_t>> {};
#endif

template<class StrT>
struct str_arg_char_default<StrT, std::enable_if_t<
    convertible_to_string_view_v<StrT, char16_t>>>
    : str_arg_char_common<char16_t, std::basic_string_view<char16_t>> {};

template<class StrT>
struct str_arg_char_default<StrT, std::enable_if_t<
    convertible_to_string_view_v<StrT, char32_t>>>
    : str_arg_char_common<char32_t, std::basic_string_view<char32_t>> {};

template<class StrT>
struct str_arg_char_default<StrT, std::enable_if_t<
    convertible_to_string_view_v<StrT, wchar_t> &&
    !convertible_to_string_view_v<StrT, char16_t> &&
    !convertible_to_string_view_v<StrT, char32_t>>>
    : str_arg_char_common<wchar_t, std::basic_string_view<wchar_t>> {};

} // namespace detail


// Requirements for string arguments

template<class StrT, typename = void>
struct str_arg_char : detail::str_arg_char_default<StrT> {};

// Null terminated string
template<class CharT>
struct str_arg_char<CharT*, std::enable_if_t<is_char_type_v<remove_cvref_t<CharT>>>> {
    using type = remove_cvref_t<CharT>;
    static constexpr str_arg<type> to_str_arg(const type* sz) {
        return { sz, std::char_traits<type>::length(sz) };
    }
};

// str_arg input
template<class CharT>
struct str_arg_char<str_arg<CharT>> {
    using type = CharT;
    static constexpr str_arg<type> to_str_arg(str_arg<type> s) {
        return s;
    }
};


// String arguments helper types

template<class StrT>
using str_arg_char_s = str_arg_char<std::decay_t<StrT>>;

template<class StrT>
using str_arg_char_t = typename str_arg_char_s<StrT>::type;


template<class StrT>
using enable_if_str_arg_t = std::enable_if_t<
    is_char_type_v<str_arg_char_t<StrT>>,
    int>;


// String arguments helper function

template <class StrT>
constexpr auto make_str_arg(StrT&& str) -> str_arg<str_arg_char_t<StrT>> {
    return str_arg_char_s<StrT>::to_str_arg(std::forward<StrT>(str));
}


// Convert to std::string or string_view

template<class CharT>
constexpr bool is_char8_type_v =
    std::is_same_v<CharT, char>
#ifdef __cpp_char8_t
    || std::is_same_v<CharT, char8_t>
#endif
;

template<class StrT>
using enable_if_str_arg_to_char8_t = std::enable_if_t<
    is_char8_type_v<str_arg_char_t<StrT>>,
    int>;

template<class CharT>
constexpr bool is_charW_type_v =
    std::is_same_v<CharT, char16_t> ||
    std::is_same_v<CharT, char32_t> ||
    std::is_same_v<CharT, wchar_t>;

template<class StrT>
using enable_if_str_arg_to_charW_t = std::enable_if_t<
    is_charW_type_v<str_arg_char_t<StrT>>,
    int>;


inline std::string&& make_string(std::string&& str) {
    return std::move(str);
}

template <class StrT, enable_if_str_arg_to_char8_t<StrT> = 0>
constexpr string_view make_string(StrT&& str) {
    const auto inp = make_str_arg(std::forward<StrT>(str));
    return { inp.data(), inp.length() };
}

template <class StrT, enable_if_str_arg_to_charW_t<StrT> = 0>
inline std::string make_string(StrT&& str) {
    const auto inp = make_str_arg(std::forward<StrT>(str));
    return url_utf::to_utf8_string(inp.begin(), inp.end());
}


} // namespace upa

#endif // UPA_STR_ARG_H
