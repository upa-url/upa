// Copyright 2026 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
module;

#define UPA_MODULE
#define UPA_EXPORT export
#define UPA_EXPORT_BEGIN export {
#define UPA_EXPORT_END }

#include <assert.h>
#include <version>

#ifndef UPA_IMPORT_STD
# ifdef __cpp_lib_modules
#  define UPA_IMPORT_STD 1
# else
#  define UPA_IMPORT_STD 0
# endif
#endif

#if !(UPA_IMPORT_STD)
# include <algorithm>
# include <array>
# include <charconv>
# include <cstddef>
# include <cstdint>
# include <filesystem>
# include <fstream>
# include <functional>
# include <initializer_list>
# include <ios>
# include <istream>
# include <iterator>
# include <limits>
# include <list>
# include <memory>
# include <optional>
# include <ostream>
# include <stdexcept>
# include <string>
# include <string_view>
# include <type_traits>
# include <unordered_map>
# include <utility>
# include <variant>
# include <vector>
#endif

export module upa.url;

#if (UPA_IMPORT_STD)
import std;
#endif

#include "upa/idna.h"
#include "upa/url.h"
#include "upa/urlpattern.h"
#include "upa/public_suffix_list.h"

// GCC still does not support private module fragments
#if !defined(__GNUC__) || defined(__clang__)
module : private;
#endif

#include "idna.cpp"
#include "url.cpp"
#include "url_ip.cpp"
#include "url_search_params.cpp"
#include "url_utf.cpp"
#include "urlpattern.cpp"
#include "unicode_id.cpp"
#include "public_suffix_list.cpp"
