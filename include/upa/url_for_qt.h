// Copyright 2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_URL_FOR_QT_H
#define UPA_URL_FOR_QT_H

#include "url.h" // IWYU pragma: export
#include <QString>

namespace upa {

template<>
struct str_arg_char<QString> {
    using type = char16_t;

    static str_arg<char16_t> to_str_arg(const QString& str) {
        return {
            reinterpret_cast<const char16_t*>(str.data()),
            static_cast<std::ptrdiff_t>(str.length())
        };
    }
};

} // namespace upa

#endif // UPA_URL_FOR_QT_H
