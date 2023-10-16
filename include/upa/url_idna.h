// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_URL_IDNA_H
#define UPA_URL_IDNA_H

#include "buffer.h"
#include "url_result.h"

namespace upa {

/// @brief Implements the domain to ASCII algorithm
///
/// See: https://url.spec.whatwg.org/#concept-domain-to-ascii
/// This function does not have a boolean @a beStrict parameter and acts as if it were false.
///
/// @param[in]  src input domain string
/// @param[in]  src_len input domain string length
/// @param[out] output buffer to store result string
/// @return `validation_errc::ok` on success, or error code on failure
validation_errc domain_to_ascii(const char16_t* src, std::size_t src_len, simple_buffer<char16_t>& output);

/// @brief Implements the domain to Unicode algorithm
///
/// See: https://url.spec.whatwg.org/#concept-domain-to-unicode
/// This function does not have a boolean @a beStrict parameter and acts as if it were false.
///
/// @param[in]  src input domain string
/// @param[in]  src_len input domain string length
/// @param[out] output buffer to store result string
/// @return `validation_errc::ok` on success, or error code on failure
validation_errc domain_to_unicode(const char* src, std::size_t src_len, simple_buffer<char>& output);

/// @brief Close the IDNA handles, conditionally close the IDNA library, and free its memory
///
/// It closes IDNA handles opened by calls to domain_to_ascii or domain_to_unicode functions.
/// If @a close_lib is `true`, it closes the IDNA library (currently it calls the `u_cleanup`
/// ICU function).
///
/// After calling this function, the `domain_to_ascii` and `domain_to_unicode` functions must
/// not be called. So it is recommended to call this function at the end of an application.
///
/// @param[in] close_lib `true` to close the IDNA library (currently ICU)
void idna_close(bool close_lib = false);

} // namespace upa

#endif // UPA_URL_IDNA_H
