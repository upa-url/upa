// Copyright 2016-2020 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

/**************************************************************
// Usage example:

template <class ...Args, enable_if_str_arg_t<Args...> = 0>
inline void procfn(Args&&... args) {
    const auto inp = make_str_arg(std::forward<Args>(args)...);
    const auto* first = inp.begin();
    const auto* last = inp.end();
    // do something with first ... last
}

**************************************************************/
#ifndef WHATWG_STR_ARG_H
#define WHATWG_STR_ARG_H

#include <cassert>
#include <iterator>
#include <string>
#include <type_traits>
#include "config.h"
#include "url_utf.h"

#ifdef WHATWG__CPP_17
# include <string_view>
# define WHATWG_URL_STR_VIEW_TYPE  std::string_view
#else
# include "str_view.h"
# define WHATWG_URL_STR_VIEW_TYPE  whatwg::str_view<char>
#endif

namespace whatwg {

// String view type

using url_str_view_t = WHATWG_URL_STR_VIEW_TYPE;

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
    str_arg(const str_arg&) = default;

    str_arg(const CharT* s)
        : first_(s)
        , last_(s + traits_type::length(s))
    {}
    str_arg(const CharT* s, std::size_t length)
        : first_(s)
        , last_(s + length)
    {}
    str_arg(const CharT* s, std::ptrdiff_t length)
        : first_(s)
        , last_(s + length)
    {}
    str_arg(const CharT* first, const CharT* last)
        : first_(first)
        , last_(last)
    {}

    // output
    constexpr const value_type* begin() const {
        return reinterpret_cast<const value_type*>(first_);
    }
    constexpr const value_type* end() const {
        return reinterpret_cast<const value_type*>(last_);
    }
    constexpr const value_type* data() const {
        return begin();
    }
    constexpr const std::size_t length() const {
        return end() - begin();
    }
    constexpr const std::size_t size() const {
        return length();
    }

protected:
    const input_type* first_;
    const input_type* last_;
};


// String type helpers

template<class T>
using remove_cvptr_t = typename std::remove_cv<typename std::remove_pointer<T>::type>::type;

namespace detail {
    // See: https://stackoverflow.com/a/9154394

    // test class T has data() member
    template<class T>
    static auto test_data(int) -> decltype(std::declval<T>().data());
    template<class>
    static auto test_data(long) -> void;

    // test class T has length() member
    template<class T>
    static auto test_length(int) -> decltype(std::declval<T>().length());
    template<class>
    static auto test_length(long) -> void;

    // T::data() return type (void - if no such member)
    template<class T>
    using data_member_t = decltype(detail::test_data<T>(0));

    // T::length() return type (void - if no such member)
    template<class T>
    using length_member_t = decltype(detail::test_length<T>(0));
} // namespace detail


// Requirements for string arguments

template<class ...Args>
struct str_arg_char {};

// two pointers
template<class CharT>
struct str_arg_char<CharT*, CharT*> : std::remove_cv<CharT> {

    template <typename T>
    static str_arg<T> to_str_arg(const T* first, const T* last) {
        assert(first <= last);
        return { first, last };
    }
};

// pointer and size
template<class CharT, class SizeT>
struct str_arg_char<CharT*, SizeT> : std::enable_if<
    is_size_type<SizeT>::value,
    typename std::remove_cv<CharT>::type> {

    template <typename T>
    static str_arg<T> to_str_arg(const T* s, std::size_t length) {
        return { s, length };
    }
};

// one pointer (null terminated string)
template<class CharT>
struct str_arg_char<CharT*> : std::remove_cv<CharT> {

    template <typename T>
    static str_arg<T> to_str_arg(const T* s) {
        return s;
    }
};

// one string class argument
template<class StrT>
struct str_arg_char<StrT> : std::enable_if<
    std::is_pointer<detail::data_member_t<StrT>>::value &&
    is_size_type<detail::length_member_t<StrT>>::value,
    remove_cvptr_t<detail::data_member_t<StrT>>> {

    template <class STR, typename T = typename STR::value_type>
    static str_arg<T> to_str_arg(const STR& str) {
        return { str.data(), str.length() };
    }
};


// String arguments helper types

template<class ...Args>
using str_arg_char_s = str_arg_char<typename std::decay<Args>::type...>;

template<class ...Args>
using str_arg_char_t = typename str_arg_char_s<Args...>::type;


template<class ...Args>
using enable_if_str_arg_t = typename std::enable_if<
    is_char_type<str_arg_char_t<Args...>>::value,
    int>::type;


// String arguments helper function

template <class ...Args>
inline auto make_str_arg(Args&&... args) -> str_arg<str_arg_char_t<Args...>> {
    return str_arg_char_s<Args...>::to_str_arg(std::forward<Args>(args)...);
}


// Convert to std::string or string_view

template<class CharT>
struct is_char8_type : std::integral_constant<bool,
    std::is_same<CharT, char>::value
#ifdef __cpp_char8_t
    || std::is_same<CharT, char8_t>::value
#endif
> {};

template<class ...Args>
using enable_if_str_arg_to_char8_t = typename std::enable_if<
    is_char8_type<str_arg_char_t<Args...>>::value,
    int>::type;

template<class CharT>
struct is_charW_type : std::integral_constant<bool,
    std::is_same<CharT, char16_t>::value ||
    std::is_same<CharT, char32_t>::value ||
    std::is_same<CharT, wchar_t>::value
> {};

template<class ...Args>
using enable_if_str_arg_to_charW_t = typename std::enable_if<
    is_charW_type<str_arg_char_t<Args...>>::value,
    int>::type;


inline std::string&& make_string(std::string&& str) {
    return std::move(str);
}

template <class ...Args, enable_if_str_arg_to_char8_t<Args...> = 0>
inline url_str_view_t make_string(Args&&... args) {
    const auto inp = make_str_arg(std::forward<Args>(args)...);
    return { inp.data(), inp.length() };
}

template <class ...Args, enable_if_str_arg_to_charW_t<Args...> = 0>
inline std::string make_string(Args&&... args) {
    const auto inp = make_str_arg(std::forward<Args>(args)...);
    return url_utf::to_utf8_string(inp.begin(), inp.end());
}


} // namespace whatwg

#endif // WHATWG_STR_ARG_H
