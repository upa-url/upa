// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

// Relative URL parsing fuzzer

#include "upa/url.h"
#include <cassert>
#include <type_traits>


// Base URLs
static const upa::url base_urls[] = {
    upa::url("http://h/p?q#f"),      // 0
    upa::url("file://h/p?q#f"),      // 1
    upa::url("non-spec://h/p?q#f"),  // 2
    // with empty host
    upa::url("file:///p?q#f"),       // 3
    upa::url("non-spec:///p?q#f"),   // 4
    // with null host
    upa::url("non-spec:/p?q#f"),     // 5
    upa::url("non-spec:p?q#f"),      // 6
    upa::url("non-spec:/.//p?q#f"),  // 7
};


template <typename T, std::size_t N>
constexpr std::size_t array_size(T (&)[N]) noexcept {
    return N;
}

static void reparse_test(const upa::url& u1) {
    upa::url u2;

    // reparse must succeed
    assert(upa::success(u2.parse(u1.href(), nullptr)));

    // reparse result must be equal to input
    assert(u2.href() == u1.href());
}

// Use libFuzzer interface
// https://llvm.org/docs/LibFuzzer.html

extern "C" int LLVMFuzzerTestOneInput(const char* data, std::size_t size) {
    // Get base URL
    if (size < 1) return 0;
    // first byte means what base URL to use:
    const auto ind = static_cast<unsigned char>(data[0]) % 0x10U;
    const upa::url* pbase = ind < array_size(base_urls) ? &base_urls[ind] : nullptr;

    // skip first byte of data
    ++data; --size;

    // Parse input data against base URL
    upa::string_view inp{ data, size };
    bool parse_ok = false;
    try {
        upa::url u1{ inp, pbase };
        parse_ok = true;
        reparse_test(u1);
    }
    catch (upa::url_error&) {
        // invalid input
    }
    catch (std::exception&) {
        assert(false);
    }

    // url::can_parse
    const bool can_parse_ok = upa::url::can_parse(inp, pbase);
    assert(can_parse_ok == parse_ok);

    return 0;
}
