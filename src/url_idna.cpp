// Copyright 2016-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
// This file contains portions of modified code from:
// https://cs.chromium.org/chromium/src/url/url_idna_icu.cc
// Copyright 2013 The Chromium Authors. All rights reserved.
//

#include "upa/config.h"
#include "upa/url_idna.h"
#include "upa/util.h"

// ICU: only C API is used (U_SHOW_CPLUSPLUS_API 0)
// https://unicode-org.github.io/icu/userguide/icu4c/build.html#icu-as-a-system-level-library
#define U_SHOW_CPLUSPLUS_API 0  // NOLINT(*-macro-*)
#include "unicode/uchar.h"  // u_getUnicodeVersion
#include "unicode/uclean.h"
#include "unicode/uidna.h"

#include <algorithm>
#include <cassert>
#include <cstdint> // uint32_t

namespace upa {

namespace {

// Return UTS46 ICU handler opened with uidna_openUTS46()

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
UIDNA* uidna_ptr = nullptr;
unsigned icu_version_major = 0;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

const UIDNA* get_uidna() {
    // initialize uidna_ptr
    static struct Once {
        Once() {
            UErrorCode err = U_ZERO_ERROR;
            // https://url.spec.whatwg.org/#idna
            // UseSTD3ASCIIRules = false
            // Transitional_Processing = false
            // CheckBidi = true
            // CheckJoiners = true
            uidna_ptr = uidna_openUTS46(
                UIDNA_CHECK_BIDI
                | UIDNA_CHECK_CONTEXTJ
                | UIDNA_NONTRANSITIONAL_TO_ASCII
                | UIDNA_NONTRANSITIONAL_TO_UNICODE, &err);
            assert(U_SUCCESS(err) && uidna_ptr != nullptr);
            // Get ICU major version
            UVersionInfo ver;
            u_getVersion(ver); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            icu_version_major = ver[0];
        }
    } const once;

    return uidna_ptr;
}

} // namespace


void idna_close(bool close_lib) {
    if (uidna_ptr) {
        uidna_close(uidna_ptr);
        uidna_ptr = nullptr;
    }
    if (close_lib) {
        // ICU cleanup
        u_cleanup();
    }
}

unsigned idna_unicode_version() {
    UVersionInfo ver;
    u_getUnicodeVersion(ver); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
    return make_unicode_version(ver[0], ver[1], ver[2], ver[3]);
}

// Conversion to ICU UChar

namespace {

static_assert(sizeof(UChar) == sizeof(char16_t), "UChar must be the same size as char16_t");

inline const UChar* to_UChar_ptr(const char16_t* p) noexcept {
    UPA_ALIASING_BARRIER(p)
    return reinterpret_cast<const UChar*>(p);
}

inline UChar* to_UChar_ptr(char16_t* p) noexcept {
    UPA_ALIASING_BARRIER(p)
    return reinterpret_cast<UChar*>(p);
}

} // namespace

// Implements the domain to ASCII algorithm
// https://url.spec.whatwg.org/#concept-domain-to-ascii
// with beStrict = false

validation_errc domain_to_ascii(const char16_t* src, std::size_t src_len, simple_buffer<char16_t>& output) {
    // https://url.spec.whatwg.org/#concept-domain-to-ascii
    // https://www.unicode.org/reports/tr46/#ToASCII
    static constexpr uint32_t UIDNA_ERR_MASK = ~static_cast<uint32_t>(
        // VerifyDnsLength = false
        UIDNA_ERROR_EMPTY_LABEL
        | UIDNA_ERROR_LABEL_TOO_LONG
        | UIDNA_ERROR_DOMAIN_NAME_TOO_LONG
        // CheckHyphens = false
        | UIDNA_ERROR_LEADING_HYPHEN
        | UIDNA_ERROR_TRAILING_HYPHEN
        | UIDNA_ERROR_HYPHEN_3_4
        );

    // uidna_nameToASCII uses int32_t length
    // https://unicode-org.github.io/icu-docs/apidoc/dev/icu4c/uidna_8h.html#ac45d3ad275df9e5a2c2e84561862d005
    if (src_len > util::unsigned_limit<int32_t>::max())
        return validation_errc::overflow; // too long

    // The static_cast<int32_t>(output.capacity()) must be safe:
    assert(output.capacity() <= util::unsigned_limit<int32_t>::max());

    const UIDNA* uidna = get_uidna();
    assert(uidna != nullptr);
    while (true) {
        UErrorCode err = U_ZERO_ERROR;
        UIDNAInfo info = UIDNA_INFO_INITIALIZER;
        const int32_t output_length = uidna_nameToASCII(uidna,
            to_UChar_ptr(src), static_cast<int32_t>(src_len),
            to_UChar_ptr(output.data()), static_cast<int32_t>(output.capacity()),
            &info, &err);
        if (U_SUCCESS(err) && (info.errors & UIDNA_ERR_MASK) == 0) {
            output.resize(output_length);
            // 3. If result is the empty string, domain-to-ASCII validation error, return failure.
            //
            // Note. Result of uidna_nameToASCII can be the empty string if input:
            // 1) consists entirely of IDNA ignored code points;
            // 2) is "xn--".
            if (output_length == 0)
                return validation_errc::domain_to_ascii;
            if (icu_version_major < 68) {
                // Workaround of ICU bug ICU-21212: https://unicode-org.atlassian.net/browse/ICU-21212
                // For some "xn--" labels which contain non ASCII chars, uidna_nameToASCII returns no error,
                // and leaves these labels unchanged in the output. Bug fixed in ICU 68.1
                if (std::any_of(output.begin(), output.end(), [](char16_t c) { return c >= 0x80; }))
                    return validation_errc::domain_to_ascii;
            }
            return validation_errc::ok;
        }

        if (err != U_BUFFER_OVERFLOW_ERROR || (info.errors & UIDNA_ERR_MASK) != 0)
            // 2. If result is a failure value, domain-to-ASCII validation error, return failure.
            return validation_errc::domain_to_ascii;

        // Not enough room in our buffer, expand.
        output.reserve(output_length);
    }
}

// Implements the domain to Unicode algorithm
// https://url.spec.whatwg.org/#concept-domain-to-unicode
// with beStrict = false

validation_errc domain_to_unicode(const char* src, std::size_t src_len, simple_buffer<char>& output) {
#if 0
    // https://url.spec.whatwg.org/#concept-domain-to-unicode
    // https://www.unicode.org/reports/tr46/#ToUnicode
    static constexpr uint32_t UIDNA_ERR_MASK = ~static_cast<uint32_t>(
        // VerifyDnsLength = false
        UIDNA_ERROR_EMPTY_LABEL
        | UIDNA_ERROR_LABEL_TOO_LONG
        | UIDNA_ERROR_DOMAIN_NAME_TOO_LONG
        // CheckHyphens = false
        | UIDNA_ERROR_LEADING_HYPHEN
        | UIDNA_ERROR_TRAILING_HYPHEN
        | UIDNA_ERROR_HYPHEN_3_4
        );
#endif

    // uidna_nameToUnicodeUTF8 uses int32_t length
    // https://unicode-org.github.io/icu-docs/apidoc/dev/icu4c/uidna_8h.html#afd9ae1e0ae5318e20c87bcb0149c3ada
    if (src_len > util::unsigned_limit<int32_t>::max())
        return validation_errc::overflow; // too long

    // The static_cast<int32_t>(output.capacity()) must be safe:
    assert(output.capacity() <= util::unsigned_limit<int32_t>::max());

    const UIDNA* uidna = get_uidna();
    assert(uidna != nullptr);
    while (true) {
        UErrorCode err = U_ZERO_ERROR;
        UIDNAInfo info = UIDNA_INFO_INITIALIZER;
        const int32_t output_length = uidna_nameToUnicodeUTF8(uidna,
            src, static_cast<int32_t>(src_len),
            output.data(), static_cast<int32_t>(output.capacity()),
            &info, &err);
        if (U_SUCCESS(err)) {
            output.resize(output_length);
            // https://url.spec.whatwg.org/#concept-domain-to-unicode
            // TODO: Signify domain-to-Unicode validation errors for any returned errors (i.e.
            // if (info.errors & UIDNA_ERR_MASK) != 0), and then, return result.
            return validation_errc::ok;
        }

        if (err != U_BUFFER_OVERFLOW_ERROR)
            return validation_errc::domain_to_unicode;

        // Not enough room in our buffer, expand.
        output.reserve(output_length);
    }
}


} // namespace upa
