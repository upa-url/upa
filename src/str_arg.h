/**************************************************************
// Usage example:

template <class CharT, typename = enable_if_char_t<CharT>>
inline bool procfn(const CharT* first, const CharT* last) {
    // do something with first ... last
}

template <class ...Args, typename = enable_if_str_arg_t<Args...>>
inline bool procfn(const Args&... args) {
    return procfn(str_arg::begin(args...), str_arg::end(args...));
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

// requirements for the arguments
template<class ...Args>
using enable_if_str_arg_t = typename std::enable_if<sizeof...(Args)>::type;

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

// character sequence with unsigned length
template<typename CharT>
inline const CharT* begin(const CharT* ptr, std::size_t) {
    return ptr;
}
template<typename CharT>
inline const CharT* end(const CharT* ptr, std::size_t len) {
    return ptr + len;
}

// character sequence with signed length
template<typename CharT>
inline const CharT* begin(const CharT* ptr, std::ptrdiff_t) {
    return ptr;
}
template<typename CharT>
inline const CharT* end(const CharT* ptr, std::ptrdiff_t len) {
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
