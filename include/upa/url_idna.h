// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_URL_IDNA_H
#define UPA_URL_IDNA_H

#include "buffer.h"
#include "url_result.h"

namespace upa {

validation_errc IDNToASCII(const char16_t* src, std::size_t src_len, simple_buffer<char16_t>& output);
validation_errc IDNToUnicode(const char* src, std::size_t src_len, simple_buffer<char>& output);
void IDNClose();

} // namespace upa

#endif // UPA_URL_IDNA_H
