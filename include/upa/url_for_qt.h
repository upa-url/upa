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
#if __has_include(<QtVersionChecks>)
# include <QtVersionChecks>
#else
# include <QtGlobal>
#endif

// As of version 6.7, Qt string classes are convertible to
// std::basic_string_view, and such strings are supported in the
// str_arg.h file.
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
#include <QString>
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
# include <QStringView>
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
# include <QUtf8StringView>
#endif

namespace upa {

template<class StrT, typename CharT>
struct str_arg_char_for_qt {
    static_assert(sizeof(typename StrT::value_type) == sizeof(CharT),
        "StrT::value_type and CharT must be the same size");
    using type = CharT;

    static str_arg<type> to_str_arg(const StrT& str) {
        return { reinterpret_cast<const type*>(str.data()), str.size() };
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

#endif // QT_VERSION < QT_VERSION_CHECK(6, 7, 0)

#endif // UPA_URL_FOR_QT_H
