// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
// This file contains portions of modified code from:
// https://cs.chromium.org/chromium/src/url/url_idna_icu.cc
// Copyright 2013 The Chromium Authors. All rights reserved.
//

#include "upa/url_idna.h"
#include "upa/util.h"
// ICU
#include "unicode/uidna.h"
#if (U_ICU_VERSION_MAJOR_NUM) >= 59
# include "unicode/char16ptr.h"
#endif
#if (U_ICU_VERSION_MAJOR_NUM) < 68
# include <algorithm>
#endif

#include <cassert>

namespace upa {

namespace {

// Return UTS46 ICU handler opened with uidna_openUTS46()

UIDNA* uidna_ptr = nullptr; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

const UIDNA* getUIDNA() {
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
        }
    } const once;

    return uidna_ptr;
}

} // namespace


void IDNClose() {
    if (uidna_ptr) {
        uidna_close(uidna_ptr);
        uidna_ptr = nullptr;
    }
}

// Conversion to ICU UChar

static_assert(sizeof(UChar) == sizeof(char16_t), "UChar must be the same size as char16_t");

#if (U_ICU_VERSION_MAJOR_NUM) < 59
// toUCharPtr functions are defined in ICU 59
namespace icu {
    inline const UChar* toUCharPtr(const char16_t* p) { return reinterpret_cast<const UChar*>(p); }
    inline UChar* toUCharPtr(char16_t* p) { return reinterpret_cast<UChar*>(p); }
}
#endif

// Converts the Unicode input representing a hostname to ASCII using IDN rules.
// The output must be ASCII, but is represented as wide characters.
//
// On success, the output will be filled with the ASCII host name and it will
// return true. Unlike most other canonicalization functions, this assumes that
// the output is empty. The beginning of the host will be at offset 0, and
// the length of the output will be set to the length of the new host name.
//
// On error, this will return false. The output in this case is undefined.
// TODO(jungshik): use UTF-8/ASCII version of nameToASCII.
// Change the function signature and callers accordingly to avoid unnecessary
// conversions in our code. In addition, consider using icu::IDNA's UTF-8/ASCII
// version with StringByteSink. That way, we can avoid C wrappers and additional
// string conversion.

// https://url.spec.whatwg.org/#concept-domain-to-ascii
// with beStrict = false

validation_errc IDNToASCII(const char16_t* src, std::size_t src_len, simple_buffer<char16_t>& output) {
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

    const UIDNA* uidna = getUIDNA();
    assert(uidna != nullptr);
    while (true) {
        UErrorCode err = U_ZERO_ERROR;
        UIDNAInfo info = UIDNA_INFO_INITIALIZER;
        const int32_t output_length = uidna_nameToASCII(uidna,
            icu::toUCharPtr(src), static_cast<int32_t>(src_len),
            icu::toUCharPtr(output.data()), static_cast<int32_t>(output.capacity()),
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
#if (U_ICU_VERSION_MAJOR_NUM) < 68
            // Workaround of ICU bug ICU-21212: https://unicode-org.atlassian.net/browse/ICU-21212
            // For some "xn--" labels which contain non ASCII chars, uidna_nameToASCII returns no error,
            // and leaves these labels unchanged in the output. Bug fixed in ICU 68.1
            if (std::any_of(output.begin(), output.end(), [](char16_t c) { return c >= 0x80; }))
                return validation_errc::domain_to_ascii;
#endif
            return validation_errc::ok;
        }

        if (err != U_BUFFER_OVERFLOW_ERROR || (info.errors & UIDNA_ERR_MASK) != 0)
            // 2. If result is a failure value, domain-to-ASCII validation error, return failure.
            return validation_errc::domain_to_ascii;

        // Not enough room in our buffer, expand.
        output.reserve(output_length);
    }
}

// TODO: common function template for IDNToASCII and IDNToUnicode

validation_errc IDNToUnicode(const char* src, std::size_t src_len, simple_buffer<char>& output) {
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

    const UIDNA* uidna = getUIDNA();
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
