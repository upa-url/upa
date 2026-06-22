// Copyright 2026 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#define UPA_MODULE
#define UPA_EXPORT export
#define UPA_EXPORT_BEGIN export {
#define UPA_EXPORT_END }

#include <assert.h>
#include <version>

#ifndef UPA_IMPORT_STD
# define UPA_IMPORT_STD 0
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
