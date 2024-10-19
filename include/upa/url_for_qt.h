// Copyright 2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.

// This file can be included instead of url.h to allow strings of the
// Qt classes QString, QStringView, QUtf8StringView to be used as string
// input in the functions of this library.
//
#ifndef UPA_URL_FOR_QT_H
#define UPA_URL_FOR_QT_H

#include "url.h" // IWYU pragma: export
#include <QString>
#ifdef __has_include
#if __has_include(<QtVersionChecks>)
# include <QtVersionChecks>
#endif
#endif // __has_include
#ifndef QT_VERSION
# include <QtGlobal>
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
# include <QStringView>
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
# include <QUtf8StringView>
#endif
#include <cstddef> // std::ptrdiff_t

namespace upa {

template<class StrT, typename CharT>
struct str_arg_char_for_qt {
    static_assert(sizeof(typename StrT::value_type) == sizeof(CharT),
        "StrT::value_type and CharT must be the same size");
    using type = CharT;

    static str_arg<type> to_str_arg(const StrT& str) {
        return {
            reinterpret_cast<const type*>(str.data()),
            static_cast<std::ptrdiff_t>(str.length())
        };
    }
};

template<>
struct str_arg_char<QString> :
    public str_arg_char_for_qt<QString, char16_t> {};

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
template<>
struct str_arg_char<QStringView> :
    public str_arg_char_for_qt<QStringView, char16_t> {};
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
template<>
struct str_arg_char<QUtf8StringView> :
    public str_arg_char_for_qt<QUtf8StringView, char> {};
#endif

} // namespace upa

#endif // UPA_URL_FOR_QT_H
