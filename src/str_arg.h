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
#include <string>
#include <type_traits>

namespace whatwg {


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
protected:
    const input_type* first_;
    const input_type* last_;
};


// Requirements for arguments
template<class ...Args>
struct is_str_arg : std::false_type {};

// two pointers
template<class CharT>
struct is_str_arg<CharT*, CharT*> : is_char_type<typename std::remove_cv<CharT>::type> {};

template<class CharT, std::size_t N>
struct is_str_arg<CharT[N], CharT*> : is_char_type<typename std::remove_cv<CharT>::type> {};

// pointer and size
template<class CharT, class SizeT>
struct is_str_arg<CharT*, SizeT> : std::integral_constant<bool,
    is_char_type<typename std::remove_cv<CharT>::type>::value &&
    is_size_type<SizeT>::value
> {};

template<class CharT, std::size_t N, class SizeT>
struct is_str_arg<CharT[N], SizeT> : std::integral_constant<bool,
    is_char_type<typename std::remove_cv<CharT>::type>::value &&
    is_size_type<SizeT>::value
> {};

// one pointer (null terminated string)
template<class CharT>
struct is_str_arg<CharT*> : is_char_type<typename std::remove_cv<CharT>::type> {};

template<class CharT, std::size_t N>
struct is_str_arg<CharT[N]> : is_char_type<typename std::remove_cv<CharT>::type> {};

// one string class argument
template<class StrT>
struct is_str_arg<StrT> : std::integral_constant<bool,
    is_char_type<typename StrT::value_type>::value &&
    std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<typename StrT::const_iterator>::iterator_category>::value
> {};

template<class CharT>
struct is_str_arg<str_arg<CharT>> : is_char_type<CharT>::type {};


// string argument helper type
template<class T>
using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template<class ...Args>
using enable_if_str_arg_t = typename std::enable_if<
    is_str_arg<remove_cvref_t<Args>...>::value,
    int>::type;


// String args helper functions

template <typename CharT>
inline str_arg<CharT> make_str_arg(const CharT* s) {
    return s;
}

template <typename CharT>
inline str_arg<CharT> make_str_arg(const CharT* s, std::size_t length) {
    return { s, length };
}

template <typename CharT>
inline str_arg<CharT> make_str_arg(const CharT* first, const CharT* last) {
    assert(first <= last);
    return { first, last };
}

template<class StrT, typename CharT = typename StrT::value_type>
inline str_arg<CharT> make_str_arg(const StrT& str) {
    return { str.data(), str.length() };
}

template <typename CharT>
inline str_arg<CharT> make_str_arg(const str_arg<CharT>& arg) {
    return arg;
}


} // namespace whatwg

#endif // WHATWG_STR_ARG_H
