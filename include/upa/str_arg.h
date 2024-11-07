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
#include <type_traits>
#include <utility>

#ifdef UPA_CPP_17
# include <string_view>
# define UPA_STRING_VIEW_TYPE  std::string_view
#else
# include "str_view.h"
# define UPA_STRING_VIEW_TYPE  upa::str_view<char>
#endif

namespace upa {

// String view type

using string_view = UPA_STRING_VIEW_TYPE;

// Supported char and size types

template<class CharT>
struct is_char_type : std::integral_constant<bool,
    std::is_same<CharT, char>::value ||
#ifdef __cpp_char8_t
    std::is_same<CharT, char8_t>::value ||
#endif
    std::is_same<CharT, char16_t>::value ||
    std::is_same<CharT, char32_t>::value ||
    std::is_same<CharT, wchar_t>::value
> {};

template<class SizeT>
struct is_size_type : std::integral_constant<bool,
    std::is_convertible<SizeT, std::size_t>::value ||
    std::is_convertible<SizeT, std::ptrdiff_t>::value
> {};


// string args helper class

template <typename CharT>
class str_arg {
public:
    using input_type = CharT;
    using traits_type = std::char_traits<input_type>;
    // output value type
    using value_type =
        // wchar_t type will be converted to char16_t or char32_t type equivalent by size
        typename std::conditional<std::is_same<CharT, wchar_t>::value, std::conditional<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>::type,
#ifdef __cpp_char8_t
        // char8_t type will be converted to char type
        typename std::conditional<std::is_same<CharT, char8_t>::value, char, input_type>::type
#else
        input_type
#endif
        >::type;

    // constructors
    str_arg(const str_arg&) noexcept = default;

    str_arg(const CharT* s)
        : first_(s)
        , last_(s + traits_type::length(s))
    {}

    template <typename SizeT, typename std::enable_if<is_size_type<SizeT>::value, int>::type = 0>
    str_arg(const CharT* s, SizeT length)
        : first_(s)
        , last_(s + length)
    { assert(length >= 0); }

    str_arg(const CharT* first, const CharT* last)
        : first_(first)
        , last_(last)
    { assert(first <= last); }

    // destructor
    ~str_arg() noexcept = default;

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
using remove_cvptr_t = typename std::remove_cv<typename std::remove_pointer<T>::type>::type;

template<class T>
using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

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
struct has_data_and_size : std::integral_constant<bool,
    std::is_pointer<data_member_t<StrT>>::value &&
    is_char_type<remove_cvptr_t<data_member_t<StrT>>>::value &&
    is_size_type<size_member_t<StrT>>::value> {};

// Default str_arg_char implementation

template<class StrT, typename = void>
struct str_arg_char_default {};

// StrT has data() and size() members
template<class StrT>
struct str_arg_char_default<StrT, typename std::enable_if<
    has_data_and_size<StrT>::value>::type> {
    using type = remove_cvptr_t<data_member_t<StrT>>;

    template <class STR>
    static str_arg<type> to_str_arg(const STR& str) {
        return { str.data(), str.size() };
    }
};

} // namespace detail


// Requirements for string arguments

template<class StrT, typename = void>
struct str_arg_char : detail::str_arg_char_default<StrT> {};

// Null terminated string
template<class CharT>
struct str_arg_char<CharT*, typename std::enable_if<
    is_char_type<remove_cvref_t<CharT>>::value>::type> {
    using type = remove_cvref_t<CharT>;

    static str_arg<type> to_str_arg(const type* s) {
        return s;
    }
};

// str_arg input
template<class CharT>
struct str_arg_char<str_arg<CharT>> {
    using type = CharT;
    static str_arg<type> to_str_arg(str_arg<type> s) {
        return s;
    }
};


// String arguments helper types

template<class StrT>
using str_arg_char_s = str_arg_char<typename std::decay<StrT>::type>;

template<class StrT>
using str_arg_char_t = typename str_arg_char_s<StrT>::type;


template<class StrT>
using enable_if_str_arg_t = typename std::enable_if<
    is_char_type<str_arg_char_t<StrT>>::value,
    int>::type;


// String arguments helper function

template <class StrT>
inline auto make_str_arg(StrT&& str) -> str_arg<str_arg_char_t<StrT>> {
    return str_arg_char_s<StrT>::to_str_arg(std::forward<StrT>(str));
}


// Convert to std::string or string_view

template<class CharT>
struct is_char8_type : std::integral_constant<bool,
    std::is_same<CharT, char>::value
#ifdef __cpp_char8_t
    || std::is_same<CharT, char8_t>::value
#endif
> {};

template<class StrT>
using enable_if_str_arg_to_char8_t = typename std::enable_if<
    is_char8_type<str_arg_char_t<StrT>>::value,
    int>::type;

template<class CharT>
struct is_charW_type : std::integral_constant<bool,
    std::is_same<CharT, char16_t>::value ||
    std::is_same<CharT, char32_t>::value ||
    std::is_same<CharT, wchar_t>::value
> {};

template<class StrT>
using enable_if_str_arg_to_charW_t = typename std::enable_if<
    is_charW_type<str_arg_char_t<StrT>>::value,
    int>::type;


inline std::string&& make_string(std::string&& str) {
    return std::move(str);
}

template <class StrT, enable_if_str_arg_to_char8_t<StrT> = 0>
inline string_view make_string(StrT&& str) {
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
