// Copyright 2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.

// This file can be included instead of url.h to allow strings of the
// ATL/MFC classes CSimpleStringT, CStringT, CFixedStringT to be used
// as string input in the functions of this library.
//
#ifndef UPA_URL_FOR_ATL_H
#define UPA_URL_FOR_ATL_H

#include "url.h" // IWYU pragma: export
#include <atlsimpstr.h>
#ifdef UPA_CPP_20
# include <concepts>
#else
# include <type_traits>
#endif

namespace upa {

template<class StrT>
struct str_arg_char_for_atl {
    using type = typename StrT::XCHAR;

    static str_arg<type> to_str_arg(const StrT& str) {
        return { str.GetString(), str.GetLength() };
    }
};

#ifdef UPA_CPP_20

// CStringT and CFixedStringT are derived from CSimpleStringT
template<class StrT> requires
    std::derived_from<StrT, ATL::CSimpleStringT<char, true>> ||
    std::derived_from<StrT, ATL::CSimpleStringT<wchar_t, true>> ||
    std::derived_from<StrT, ATL::CSimpleStringT<char, false>> ||
    std::derived_from<StrT, ATL::CSimpleStringT<wchar_t, false>>
struct str_arg_char<StrT> : public str_arg_char_for_atl<StrT> {};

#else // UPA_CPP_20

// CStringT and CFixedStringT are derived from CSimpleStringT
template<class StrT>
struct str_arg_char<StrT, std::enable_if_t<
    is_derived_from_v<StrT, ATL::CSimpleStringT<char, true>> ||
    is_derived_from_v<StrT, ATL::CSimpleStringT<wchar_t, true>> ||
    is_derived_from_v<StrT, ATL::CSimpleStringT<char, false>> ||
    is_derived_from_v<StrT, ATL::CSimpleStringT<wchar_t, false>>
    >> : public str_arg_char_for_atl<StrT> {};

#endif // UPA_CPP_20

} // namespace upa

#endif // UPA_URL_FOR_ATL_H
