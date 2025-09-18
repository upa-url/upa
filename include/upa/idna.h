// Copyright 2017-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_H
#define UPA_IDNA_H

// #include "idna/idna.h"
// Copyright 2017-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_IDNA_H
#define UPA_IDNA_IDNA_H

// #include "bitmask_operators.hpp"
#ifndef UPA_IDNA_BITMASK_OPERATORS_HPP
#define UPA_IDNA_BITMASK_OPERATORS_HPP

// (C) Copyright 2015 Just Software Solutions Ltd
//
// Distributed under the Boost Software License, Version 1.0.
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or
// organization obtaining a copy of the software and accompanying
// documentation covered by this license (the "Software") to use,
// reproduce, display, distribute, execute, and transmit the
// Software, and to prepare derivative works of the Software, and
// to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire
// statement, including the above license grant, this restriction
// and the following disclaimer, must be included in all copies
// of the Software, in whole or in part, and all derivative works
// of the Software, unless such copies or derivative works are
// solely in the form of machine-executable object code generated
// by a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
// KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
// COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE
// LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include<type_traits>

namespace upa::idna {

template<typename E>
struct enable_bitmask_operators : public std::false_type {};

template<typename E>
constexpr bool enable_bitmask_operators_v = enable_bitmask_operators<E>::value;

} // namespace upa::idna

template<typename E>
constexpr std::enable_if_t<upa::idna::enable_bitmask_operators_v<E>, E>
operator|(E lhs, E rhs) noexcept {
    using underlying = std::underlying_type_t<E>;
    return static_cast<E>(
        static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template<typename E>
constexpr std::enable_if_t<upa::idna::enable_bitmask_operators_v<E>, E>
operator&(E lhs, E rhs) noexcept {
    using underlying = std::underlying_type_t<E>;
    return static_cast<E>(
        static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template<typename E>
constexpr std::enable_if_t<upa::idna::enable_bitmask_operators_v<E>, E>
operator^(E lhs, E rhs) noexcept {
    using underlying = std::underlying_type_t<E>;
    return static_cast<E>(
        static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

template<typename E>
constexpr std::enable_if_t<upa::idna::enable_bitmask_operators_v<E>, E>
operator~(E lhs) noexcept {
    using underlying = std::underlying_type_t<E>;
    return static_cast<E>(
        ~static_cast<underlying>(lhs));
}

template<typename E>
constexpr std::enable_if_t<upa::idna::enable_bitmask_operators_v<E>, E&>
operator|=(E& lhs, E rhs) noexcept {
    using underlying = std::underlying_type_t<E>;
    lhs = static_cast<E>(
        static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
    return lhs;
}

template<typename E>
constexpr std::enable_if_t<upa::idna::enable_bitmask_operators_v<E>, E&>
operator&=(E& lhs, E rhs) noexcept {
    using underlying = std::underlying_type_t<E>;
    lhs = static_cast<E>(
        static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
    return lhs;
}

template<typename E>
constexpr std::enable_if_t<upa::idna::enable_bitmask_operators_v<E>, E&>
operator^=(E& lhs, E rhs) noexcept {
    using underlying = std::underlying_type_t<E>;
    lhs = static_cast<E>(
        static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
    return lhs;
}

#endif // UPA_IDNA_BITMASK_OPERATORS_HPP

// #include "config.h"
// Copyright 2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_CONFIG_H
#define UPA_IDNA_CONFIG_H

// Define UPA_IDNA_API macro to mark symbols for export/import
// when compiling as shared library
#if defined (UPA_LIB_EXPORT) || defined (UPA_LIB_IMPORT)
# ifdef _MSC_VER
#  ifdef UPA_LIB_EXPORT
#   define UPA_IDNA_API __declspec(dllexport)
#  else
#   define UPA_IDNA_API __declspec(dllimport)
#  endif
# elif defined(__clang__) || defined(__GNUC__)
#  define UPA_IDNA_API __attribute__((visibility ("default")))
# endif
#endif
#ifndef UPA_IDNA_API
# define UPA_IDNA_API
#endif

#endif // UPA_IDNA_CONFIG_H
 // IWYU pragma: export
// #include "idna_version.h"
// Copyright 2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_IDNA_VERSION_H
#define UPA_IDNA_IDNA_VERSION_H

// NOLINTBEGIN(*-macro-*)

#define UPA_IDNA_VERSION_MAJOR 2
#define UPA_IDNA_VERSION_MINOR 4
#define UPA_IDNA_VERSION_PATCH 0

#define UPA_IDNA_VERSION "2.4.0"

// NOLINTEND(*-macro-*)

#endif // UPA_IDNA_IDNA_VERSION_H
 // IWYU pragma: export
#include <string>

namespace upa::idna {

enum class Option {
    Default           = 0,
    UseSTD3ASCIIRules = 0x0001,
    Transitional      = 0x0002,
    VerifyDnsLength   = 0x0004,
    CheckHyphens      = 0x0008,
    CheckBidi         = 0x0010,
    CheckJoiners      = 0x0020,
    // ASCII optimization
    InputASCII        = 0x1000,
};

template<>
struct enable_bitmask_operators<Option> : public std::true_type {};


namespace detail {

// Bit flags
constexpr bool has(Option option, const Option value) noexcept {
    return (option & value) == value;
}

constexpr Option domain_options(bool be_strict, bool is_input_ascii) noexcept {
    // https://url.spec.whatwg.org/#concept-domain-to-ascii
    // https://url.spec.whatwg.org/#concept-domain-to-unicode
    // Note. The to_unicode ignores Option::VerifyDnsLength
    auto options = Option::CheckBidi | Option::CheckJoiners;
    if (be_strict)
        options |= Option::CheckHyphens | Option::UseSTD3ASCIIRules | Option::VerifyDnsLength;
    if (is_input_ascii)
        options |= Option::InputASCII;
    return options;
}

// IDNA map and normalize to NFC

template <typename CharT>
bool map(std::u32string& mapped, const CharT* input, const CharT* input_end, Option options, bool is_to_ascii);

extern template UPA_IDNA_API bool map(std::u32string&, const char*, const char*, Option, bool);
extern template UPA_IDNA_API bool map(std::u32string&, const char16_t*, const char16_t*, Option, bool);
extern template UPA_IDNA_API bool map(std::u32string&, const char32_t*, const char32_t*, Option, bool);

// Performs ToASCII on IDNA-mapped and normalized to NFC input
UPA_IDNA_API bool to_ascii_mapped(std::string& domain, const std::u32string& mapped, Option options);

// Performs ToUnicode on IDNA-mapped and normalized to NFC input
UPA_IDNA_API bool to_unicode_mapped(std::u32string& domain, const std::u32string& mapped, Option options);

} // namespace detail


/// @brief Implements the Unicode IDNA ToASCII
///
/// See: https://www.unicode.org/reports/tr46/#ToASCII
///
/// @param[out] domain buffer to store result string
/// @param[in]  input source domain string
/// @param[in]  input_end the end of source domain string
/// @param[in]  options
/// @return `true` on success, or `false` on failure
template <typename CharT>
inline bool to_ascii(std::string& domain, const CharT* input, const CharT* input_end, Option options) {
    // P1 - Map and further processing
    std::u32string mapped;
    domain.clear();
    return
        detail::map(mapped, input, input_end, options, true) &&
        detail::to_ascii_mapped(domain, mapped, options);
}

/// @brief Implements the Unicode IDNA ToUnicode
///
/// See: https://www.unicode.org/reports/tr46/#ToUnicode
///
/// @param[out] domain buffer to store result string
/// @param[in]  input source domain string
/// @param[in]  input_end the end of source domain string
/// @param[in]  options
/// @return `true` on success, or `false` on errors
template <typename CharT>
inline bool to_unicode(std::u32string& domain, const CharT* input, const CharT* input_end, Option options) {
    // P1 - Map and further processing
    std::u32string mapped;
    detail::map(mapped, input, input_end, options, false);
    return detail::to_unicode_mapped(domain, mapped, options);
}

/// @brief Implements the domain to ASCII algorithm
///
/// See: https://url.spec.whatwg.org/#concept-domain-to-ascii
///
/// @param[out] domain buffer to store result string
/// @param[in]  input source domain string
/// @param[in]  input_end the end of source domain string
/// @param[in]  be_strict
/// @param[in]  is_input_ascii
/// @return `true` on success, or `false` on failure
template <typename CharT>
inline bool domain_to_ascii(std::string& domain, const CharT* input, const CharT* input_end,
    bool be_strict = false, bool is_input_ascii = false)
{
    const bool res = to_ascii(domain, input, input_end, detail::domain_options(be_strict, is_input_ascii));

    // 3. If result is the empty string, domain-to-ASCII validation error, return failure.
    //
    // Note. Result of to_ascii can be the empty string if input consists entirely of
    // IDNA ignored code points.
    return res && !domain.empty();
}

/// @brief Implements the domain to Unicode algorithm
///
/// See: https://url.spec.whatwg.org/#concept-domain-to-unicode
///
/// @param[out] domain buffer to store result string
/// @param[in]  input source domain string
/// @param[in]  input_end the end of source domain string
/// @param[in]  be_strict
/// @param[in]  is_input_ascii
/// @return `true` on success, or `false` on errors
template <typename CharT>
inline bool domain_to_unicode(std::u32string& domain, const CharT* input, const CharT* input_end,
    bool be_strict = false, bool is_input_ascii = false)
{
    return to_unicode(domain, input, input_end, detail::domain_options(be_strict, is_input_ascii));
}

/// @brief Encodes Unicode version
///
/// The version is encoded as follows: <version 1st number> * 0x1000000 +
/// <version 2nd number> * 0x10000 + <version 3rd number> * 0x100 + <version 4th number>
///
/// For example for Unicode version 15.1.0 it returns 0x0F010000
///
/// @param[in] n1 version 1st number
/// @param[in] n2 version 2nd number
/// @param[in] n3 version 3rd number
/// @param[in] n4 version 4th number
/// @return encoded Unicode version
[[nodiscard]] constexpr unsigned make_unicode_version(unsigned n1, unsigned n2 = 0,
    unsigned n3 = 0, unsigned n4 = 0) noexcept {
    return n1 << 24 | n2 << 16 | n3 << 8 | n4;
}

/// @brief Gets Unicode version that IDNA library conforms to
///
/// @return encoded Unicode version
/// @see make_unicode_version
[[nodiscard]] inline unsigned unicode_version() {
    return make_unicode_version(17);
}


} // namespace upa::idna

#endif // UPA_IDNA_IDNA_H
 // IWYU pragma: export
// #include "idna/nfc.h"
// Copyright 2024-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_NFC_H
#define UPA_IDNA_NFC_H

// #include "config.h"
 // IWYU pragma: export
// #include <string>

namespace upa::idna {


UPA_IDNA_API void compose(std::u32string& str);
UPA_IDNA_API void canonical_decompose(std::u32string& str);

UPA_IDNA_API void normalize_nfc(std::u32string& str);
[[nodiscard]] UPA_IDNA_API bool is_normalized_nfc(const char32_t* first, const char32_t* last);


} // namespace upa::idna

#endif // UPA_IDNA_NFC_H

// #include "idna/punycode.h"
// Copyright 2017-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_PUNYCODE_H
#define UPA_IDNA_PUNYCODE_H

// #include "config.h"
 // IWYU pragma: export
// #include <string>

namespace upa::idna::punycode {

enum class status {
    success = 0,
    bad_input = 1,  // Input is invalid.
    big_output = 2, // Output would exceed the space provided.
    overflow = 3    // Wider integers needed to process input.
};

UPA_IDNA_API status encode(std::string& output, const char32_t* first, const char32_t* last);
UPA_IDNA_API status decode(std::u32string& output, const char32_t* first, const char32_t* last);

} // namespace upa::idna::punycode

#endif // UPA_IDNA_PUNYCODE_H


#endif // UPA_IDNA_H
