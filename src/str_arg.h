// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

/**************************************************************
// Usage example:

template <class CharT, enable_if_char_t<CharT, int> = 0>
inline bool procfn(const CharT* first, const CharT* last) {
    // do something with first ... last
}

template <class ...Args, enable_if_pstr_arg_t<Args...> = 0>
inline bool procfn(Args... args) {
    return procfn(str_arg::begin(args...), str_arg::end(args...));
}

template <class StrT, enable_if_str_arg_t<StrT> = 0>
inline bool procfn(const StrT& str) {
    return procfn(str_arg::begin(str), str_arg::end(str));
}

**************************************************************/
#ifndef WHATWG_STR_ARG_H
#define WHATWG_STR_ARG_H

#include <string>
#include <type_traits>

namespace whatwg {

// Enabled processing char types (char, char16_t, char32_t)
template<class CharT, class T = void>
using enable_if_char_t = typename std::enable_if<
    std::is_same<CharT, char>::value ||
    std::is_same<CharT, char16_t>::value ||
    std::is_same<CharT, char32_t>::value,
    T>::type;

// supported char and size types
template<class CharT>
struct is_char_type : std::integral_constant<bool,
    std::is_same<CharT, char>::value ||
    std::is_same<CharT, char16_t>::value ||
    std::is_same<CharT, char32_t>::value ||
    std::is_same<CharT, wchar_t>::value
> {};

template<class SizeT>
struct is_size_type : std::integral_constant<bool,
    std::is_convertible<SizeT, std::size_t>::value ||
    std::is_convertible<SizeT, std::ptrdiff_t>::value
> {};


// Requirements for arguments
template<class ...Args>
struct is_pstr_arg : std::false_type {};

// two arguments
template<class T1, class T2>
struct is_pstr_arg<T1*, T2*> : std::integral_constant<bool,
    std::is_same<typename std::remove_cv<T1>::type, wchar_t>::value &&
    std::is_same<typename std::remove_cv<T2>::type, wchar_t>::value
> {};

template<class CharT, class SizeT>
struct is_pstr_arg<CharT*, SizeT> : std::integral_constant<bool,
    is_char_type<typename std::remove_cv<CharT>::type>::value &&
    is_size_type<SizeT>::value
> {};

// one argument
template<class CharT>
struct is_pstr_arg<CharT*> : is_char_type<typename std::remove_cv<CharT>::type> {};

// string arguments helper types
template<class ...Args>
using enable_if_pstr_arg_t = typename std::enable_if<is_pstr_arg<Args...>::value, int>::type;

template<class StrT>
using enable_if_str_arg_t = typename std::enable_if<
    is_char_type<typename StrT::value_type>::value &&
    std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<typename StrT::const_iterator>::iterator_category>::value,
    int>::type;


// Alias of char16_t or char32_t type equivalent to wchar_t type by size
using wchar_char_t = std::conditional<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>::type;


// Helpers to get the starting and ending pointers of various string args
namespace str_arg {

// converts wchar_t* to wchar_char_t*
inline const wchar_char_t* begin(const wchar_t* ptr, const wchar_t*) {
    return reinterpret_cast<const wchar_char_t*>(ptr);
}
inline const wchar_char_t* end(const wchar_t*, const wchar_t* ptr) {
    return reinterpret_cast<const wchar_char_t*>(ptr);
}

// character sequence with length
template<typename CharT, typename SizeT, typename std::enable_if<is_size_type<SizeT>::value, int>::type = 0>
inline const CharT* begin(const CharT* ptr, SizeT) {
    return ptr;
}
template<typename CharT, typename SizeT, typename std::enable_if<is_size_type<SizeT>::value, int>::type = 0>
inline const CharT* end(const CharT* ptr, SizeT len) {
    return ptr + len;
}

// null terminated character sequence
template<typename CharT>
inline const CharT* begin(const CharT* ptr) {
    return ptr;
}
template<typename CharT>
inline const CharT* end(const CharT* ptr) {
    return ptr + std::char_traits<CharT>::length(ptr);
}

// string class with data() and length() member functions
template<class StrT, typename CharT = typename StrT::value_type>
inline const CharT* begin(const StrT& str) {
    return str.data();
}
template<class StrT, typename CharT = typename StrT::value_type>
inline const CharT* end(const StrT& str) {
    return str.data() + str.length();
}


} // namespace str_arg
} // namespace whatwg

#endif // WHATWG_STR_ARG_H
