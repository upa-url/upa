// Copyright 2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_URL_FOR_ATL_H
#define UPA_URL_FOR_ATL_H

#include "url.h" // IWYU pragma: export
#include <cstringt.h>

namespace upa {

template<typename CharT, class StringTraits>
struct str_arg_char<ATL::CStringT<CharT, StringTraits>> {
    using type = CharT;

    static str_arg<CharT> to_str_arg(const ATL::CStringT<CharT, StringTraits>& str) {
        return { str.GetString(), static_cast<std::ptrdiff_t>(str.GetLength()) };
    }
};

} // namespace upa

#endif // UPA_URL_FOR_ATL_H
