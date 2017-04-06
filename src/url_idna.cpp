
#include "url_idna.h"
#include <cassert>
#include <limits>
// ICU
#include "unicode/uidna.h"

namespace whatwg {

// https://cs.chromium.org/chromium/src/url/url_canon_icu.cc
// https://cs.chromium.org/chromium/src/url/url_canon_icu.h

namespace {

// A wrapper to ICU's UIDNA, a C pointer to a UTS46/IDNA 2008 handling
// object opened with uidna_openUTS46().
//
// We use UTS46 with BiDiCheck to migrate from IDNA 2003 (with unassigned
// code points allowed) to IDNA 2008 with
// the backward compatibility in mind. What it does:
//
// 1. Use the up-to-date Unicode data.
// 2. Define a case folding/mapping with the up-to-date Unicode data as
//    in IDNA 2003.
// 3. Use transitional mechanism for 4 deviation characters (sharp-s,
//    final sigma, ZWJ and ZWNJ) for now.
// 4. Continue to allow symbols and punctuations.
// 5. Apply new BiDi check rules more permissive than the IDNA 2003 BiDI rules.
// 6. Do not apply STD3 rules
// 7. Do not allow unassigned code points.
//
// It also closely matches what IE 10 does except for the BiDi check (
// http://goo.gl/3XBhqw ).
// See http://unicode.org/reports/tr46/ and references therein
// for more details.
struct UIDNAWrapper {
    UIDNAWrapper() {
        UErrorCode err = U_ZERO_ERROR;
        // TODO(jungshik): Change options as different parties (browsers,
        // registrars, search engines) converge toward a consensus.
        // UIDNA_NONTRANSITIONAL_TO_UNICODE: see module.exports.toUnicode(..)
        // at: https://github.com/Sebmaster/tr46.js/blob/master/index.js
        value = uidna_openUTS46(UIDNA_CHECK_BIDI
            | UIDNA_NONTRANSITIONAL_TO_ASCII
            | UIDNA_NONTRANSITIONAL_TO_UNICODE, &err);
        if (U_FAILURE(err)) {
            //todo: CHECK(false) << "failed to open UTS46 data with error: " << err;
            value = nullptr;
        }
    }

    UIDNA* value;
};

} // namespace


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

static_assert(sizeof(char16_t) == sizeof(UChar), "");

bool IDNToASCII(const char16_t* src, size_t src_len, simple_buffer<char16_t>& output) {
    // TODO: inicializavimas
    static UIDNAWrapper g_uidna;

    // https://url.spec.whatwg.org/#concept-domain-to-ascii
    // http://www.unicode.org/reports/tr46/#ToASCII
    // VerifyDnsLength = false
    static const uint32_t UIDNA_ERR_MASK = ~(uint32_t)(
        UIDNA_ERROR_EMPTY_LABEL | UIDNA_ERROR_LABEL_TOO_LONG |
        UIDNA_ERROR_DOMAIN_NAME_TOO_LONG
        );

    // uidna_nameToASCII uses int32_t length
    // http://icu-project.org/apiref/icu4c/uidna_8h.html#a9cc0383836cc8b73d14e86d5014ee7ae
    if (src_len > std::numeric_limits<int32_t>::max())
        return false; // too long

    UIDNA* uidna = g_uidna.value;
    assert(uidna != nullptr);
    while (true) {
        UErrorCode err = U_ZERO_ERROR;
        UIDNAInfo info = UIDNA_INFO_INITIALIZER;
        int output_length = uidna_nameToASCII(uidna,
            (const UChar*)src, static_cast<int32_t>(src_len),
            (UChar*)output.data(), output.capacity(),
            &info, &err);
        if (U_SUCCESS(err) && (info.errors & UIDNA_ERR_MASK) == 0) {
            output.resize(output_length);
            return true;
        }

        if (err != U_BUFFER_OVERFLOW_ERROR || (info.errors & UIDNA_ERR_MASK) != 0)
            return false;  // Unknown error, give up.

        // Not enough room in our buffer, expand.
        output.reserve(output_length);
    }
}

// TODO: common function template for IDNToASCII and IDNToUnicode

bool IDNToUnicode(const char* src, size_t src_len, simple_buffer<char>& output) {
    // TODO: inicializavimas
    static UIDNAWrapper g_uidna;

    // uidna_nameToUnicodeUTF8 uses int32_t length
    // http://icu-project.org/apiref/icu4c/uidna_8h.html#a61648a995cff1f8d626df1c16ad4f3b8
    if (src_len > std::numeric_limits<int32_t>::max())
        return false; // too long

    UIDNA* uidna = g_uidna.value;
    assert(uidna != nullptr);
    while (true) {
        UErrorCode err = U_ZERO_ERROR;
        UIDNAInfo info = UIDNA_INFO_INITIALIZER;
        int output_length = uidna_nameToUnicodeUTF8(uidna,
            src, static_cast<int32_t>(src_len),
            output.data(), output.capacity(),
            &info, &err);
        if (U_SUCCESS(err)) {
            output.resize(output_length);
            return true;
        }

        // TODO: Signify validation errors for any returned errors, and then, return result
        // https://url.spec.whatwg.org/#concept-domain-to-unicode
        // TODO(jungshik): Look at info.errors to handle them case-by-case basis
        // if necessary.
        if (err != U_BUFFER_OVERFLOW_ERROR)
            return false;  // Unknown error, give up.

        // Not enough room in our buffer, expand.
        output.reserve(output_length);
    }
}


} // namespace whatwg
