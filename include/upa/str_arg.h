// Copyright 2016-2024 Rimas Misevičius
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

template <typename CharT>
class str_arg {
public:
    using input_type = CharT;
    using traits_type = std::char_traits<input_type>;
    // output value type
    using value_type =
        // wchar_t type will be converted to char16_t or char32_t type equivalent by size
        std::conditional_t<std::is_same_v<CharT, wchar_t>, std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>,
#ifdef __cpp_char8_t
        // char8_t type will be converted to char type
        std::conditional_t<std::is_same_v<CharT, char8_t>, char, input_type>
#else
        input_type
#endif
        >;

    // constructors
    constexpr str_arg(const str_arg&) noexcept = default;

    constexpr str_arg(const CharT* s)
        : first_(s)
        , last_(s + traits_type::length(s))
    {}

    template <typename SizeT, std::enable_if_t<is_size_type_v<SizeT>, int> = 0>
    constexpr str_arg(const CharT* s, SizeT length)
        : first_(s)
        , last_(s + length)
    { assert(length >= 0); }

    constexpr str_arg(const CharT* first, const CharT* last)
        : first_(first)
        , last_(last)
    { assert(first <= last); }

    // destructor
    UPA_CONSTEXPR_20 ~str_arg() noexcept = default;

    // assignment is not used
    str_arg& operator=(const str_arg&) = delete;

    // output
    constexpr const value_type* begin() const noexcept {
        return reinterpret_cast<const value_type*>(first_);
    }
    constexpr const value_type* end() const noexcept {
        return reinterpret_cast<const value_type*>(last_);
    }
    constexpr const value_type* data() const noexcept {
        return begin();
    }
    constexpr std::size_t length() const noexcept {
        return end() - begin();
    }
    constexpr std::size_t size() const noexcept {
        return length();
    }

private:
    const input_type* first_;
    const input_type* last_;
};


// String type helpers

template<class T>
using remove_cvptr_t = std::remove_cv_t<std::remove_pointer_t<T>>;

template<class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

namespace detail {

// See: https://stackoverflow.com/a/9154394

// test class T has data() member
template<class T>
auto test_data(int) -> decltype(std::declval<T>().data());
template<class>
auto test_data(long) -> void;

// test class T has size() member
template<class T>
auto test_size(int) -> decltype(std::declval<T>().size());
template<class>
auto test_size(long) -> void;

// T::data() return type (void - if no such member)
template<class T>
using data_member_t = decltype(detail::test_data<T>(0));

// T::size() return type (void - if no such member)
template<class T>
using size_member_t = decltype(detail::test_size<T>(0));

// Check that StrT has data() and size() members of supported types

template<class StrT>
constexpr bool has_data_and_size_v =
    std::is_pointer_v<detail::data_member_t<StrT>> &&
    is_char_type_v<remove_cvptr_t<detail::data_member_t<StrT>>> &&
    is_size_type_v<detail::size_member_t<StrT>>;

// Check StrT is convertible to std::basic_string_view

template<class StrT, typename CharT>
constexpr bool convertible_to_string_view_v =
    std::is_convertible_v<StrT, std::basic_string_view<CharT>> &&
    !has_data_and_size_v<StrT> &&
    !std::is_same_v<StrT, std::nullptr_t>;

// Common class for converting input to str_arg

template<typename CharT, class ArgT>
struct str_arg_char_common {
    using type = CharT;
    static constexpr str_arg<CharT> to_str_arg(ArgT str) {
        return { str.data(), str.size() };
    }
};

// Default str_arg_char implementation

template<class StrT, typename = void>
struct str_arg_char_default {};

// StrT has data() and size() members
template<class StrT>
struct str_arg_char_default<StrT, std::enable_if_t<
    has_data_and_size_v<StrT>>>
    : str_arg_char_common<
    remove_cvptr_t<detail::data_member_t<StrT>>,
    remove_cvref_t<StrT> const&> {};

// StrT is convertible to std::basic_string_view
template<class StrT>
struct str_arg_char_default<StrT, std::enable_if_t<
    convertible_to_string_view_v<StrT, char>>>
    : str_arg_char_common<char, std::basic_string_view<char>> {};

#ifdef __cpp_char8_t
template<class StrT>
struct str_arg_char_default<StrT, std::enable_if_t<
    convertible_to_string_view_v<StrT, char8_t>>>
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
    convertible_to_string_view_v<StrT, wchar_t>>>
    : str_arg_char_common<wchar_t, std::basic_string_view<wchar_t>> {};

} // namespace detail


// Requirements for string arguments

template<class StrT, typename = void>
struct str_arg_char : detail::str_arg_char_default<StrT> {};

// Null terminated string
template<class CharT>
struct str_arg_char<CharT*, std::enable_if_t<is_char_type_v<remove_cvref_t<CharT>>>> {
    using type = remove_cvref_t<CharT>;
    static constexpr str_arg<type> to_str_arg(const type* s) {
        return s;
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
