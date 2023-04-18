// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef WHATWG_UTIL_H
#define WHATWG_UTIL_H

#include "config.h"
#include <stdexcept>
#include <string>

namespace whatwg { // NOLINT(modernize-concat-nested-namespaces)
namespace util {


template <class StrT>
inline void append(std::string& dest, const StrT& src) {
#ifdef _MSC_VER
    if (dest.max_size() - dest.size() < src.size())
        throw std::length_error("too big size");
    // now it is safe to add sizes
    dest.reserve(dest.size() + src.size());
    for (const auto c : src)
        dest.push_back(static_cast<char>(c));
#else
    dest.append(src.begin(), src.end());
#endif
}


} // namespace util
} // namespace whatwg

#endif // WHATWG_UTIL_H
