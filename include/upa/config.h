// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_CONFIG_H
#define UPA_CONFIG_H

#ifdef __has_include
# if __has_include(<version>)
#  include <version>
# endif
#endif

// Define UPA_CPP_20 if compiler supports C++20 or later standard
#if defined(_MSVC_LANG) ? (_MSVC_LANG >= 202002) : (__cplusplus >= 202002)
# define UPA_CPP_20
#endif

// Define UPA_CPP_17 if compiler supports C++17 or later standard
// https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
#if defined(_MSVC_LANG) ? (_MSVC_LANG >= 201703) : (__cplusplus >= 201703)
# define UPA_CPP_17
# define UPA_FALLTHROUGH [[fallthrough]];
# define UPA_CONSTEXPR_17 constexpr
# define UPA_NOEXCEPT_17 noexcept
#else
# define UPA_FALLTHROUGH
# define UPA_CONSTEXPR_17 inline
# define UPA_NOEXCEPT_17
#endif

// Define UPA_CPP_14 if compiler supports C++14 or later
#if defined(_MSVC_LANG) ? (_MSVC_LANG >= 201402) : (__cplusplus >= 201402)
# define UPA_CPP_14
# define UPA_CONSTEXPR_14 constexpr
#else
# define UPA_CONSTEXPR_14 inline
#endif

#endif // UPA_CONFIG_H
