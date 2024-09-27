// Copyright 2016-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_CONFIG_H
#define UPA_CONFIG_H

#ifdef __has_include
# if __has_include(<version>)
#  include <version> // IWYU pragma: export
# endif
#endif

// Define UPA_CPP_20 if compiler supports C++20 or later standard
// https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
#if defined(_MSVC_LANG) ? (_MSVC_LANG >= 202002) : (__cplusplus >= 202002)
# define UPA_CPP_20
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
