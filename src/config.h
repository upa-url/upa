// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef WHATWG_CONFIG_H
#define WHATWG_CONFIG_H

// Define WHATWG__CPP_17 if compiler supports C++17 or later standard
// https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
#if defined(_MSVC_LANG) ? (_MSVC_LANG >= 201703) : (__cplusplus >= 201703)
#define WHATWG__CPP_17
#endif

// Define WHATWG__CPP_14 if compiler supports C++14 or later
#if defined(_MSVC_LANG) ? (_MSVC_LANG >= 201402) : (__cplusplus >= 201402)
#define WHATWG__CPP_14
#endif

#endif // WHATWG_CONFIG_H
