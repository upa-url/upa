// Copyright 2016-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_CONFIG_H
#define UPA_CONFIG_H

#if __has_include(<version>)
# include <version> // IWYU pragma: export
#endif

// Macros for compilers that support the C++20 or later standard
// https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
#if defined(_MSVC_LANG) ? (_MSVC_LANG >= 202002) : (__cplusplus >= 202002)
# define UPA_CPP_20
# define UPA_CONSTEXPR_20 constexpr
#else
# define UPA_CONSTEXPR_20 inline
#endif

// Define UPA_API macro to mark symbols for export/import
// when compiling as shared library
#if defined (UPA_LIB_EXPORT) || defined (UPA_LIB_IMPORT)
# ifdef _MSC_VER
#  ifdef UPA_LIB_EXPORT
#   define UPA_API __declspec(dllexport)
#  else
#   define UPA_API __declspec(dllimport)
#  endif
# elif defined(__clang__) || defined(__GNUC__)
#  define UPA_API __attribute__((visibility ("default")))
# endif
#endif
#ifndef UPA_API
# define UPA_API
#endif

// Barrier for pointer anti-aliasing optimizations even across function boundaries.
// This is a slightly modified U_ALIASING_BARRIER macro from the char16ptr.h file
// of the ICU 75.1 library.
// Discussion: https://github.com/sg16-unicode/sg16/issues/67
#ifndef UPA_ALIASING_BARRIER
# if defined(__clang__) || defined(__GNUC__)
#  define UPA_ALIASING_BARRIER(ptr) asm volatile("" : : "rm"(ptr) : "memory");  // NOLINT(*-macro-*,hicpp-no-assembler)
# else
#  define UPA_ALIASING_BARRIER(ptr)
# endif
#endif

#endif // UPA_CONFIG_H
