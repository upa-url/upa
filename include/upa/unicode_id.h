// Copyright 2023-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_UNICODE_ID_H
#define UPA_UNICODE_ID_H

#include "upa/config.h"

namespace upa::pattern::table {

UPA_API bool is_identifier_start(char32_t cp) noexcept;
UPA_API bool is_identifier_part(char32_t cp) noexcept;

} // namespace upa::pattern::table

#endif // UPA_UNICODE_ID_H
