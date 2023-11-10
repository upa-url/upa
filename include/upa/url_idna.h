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

/// @brief Gets Unicode version that IDNA library conforms to
///
/// @return encoded Unicode version
/// @see make_unicode_version
unsigned idna_unicode_version();

/// @brief Encodes Unicode version
///
/// The version is encoded as follows: <version 1st number> * 0x1000000 +
/// <version 2nd number> * 0x10000 + <version 3rd number> * 0x100 + <version 4th number>
///
/// For example for Unicode version 15.1.0 it returns 0x0F010000
///
/// @param[in] n1 version 1st number
/// @param[in] n2 version 2nd number
/// @param[in] n3 version 3rd number
/// @param[in] n4 version 4th number
/// @return encoded Unicode version
constexpr unsigned make_unicode_version(unsigned n1, unsigned n2 = 0,
    unsigned n3 = 0, unsigned n4 = 0) {
    return n1 << 24 | n2 << 16 | n3 << 8 | n4;
}

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
