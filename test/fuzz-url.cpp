// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

// Relative URL parsing fuzzer

#include "url.h"
#include <cassert>
#include <type_traits>


// Base URLs
static const whatwg::url base_urls[] = {
    whatwg::url("http://h/p?q#f"),      // 0
    whatwg::url("file://h/p?q#f"),      // 1
    whatwg::url("non-spec://h/p?q#f"),  // 2
    // with empty host
    whatwg::url("file:///p?q#f"),       // 3
    whatwg::url("non-spec:///p?q#f"),   // 4
    // with null host
    whatwg::url("non-spec:/p?q#f"),     // 5
    whatwg::url("non-spec:p?q#f"),      // 6
    whatwg::url("non-spec:/.//p?q#f"),  // 7
};


template <typename T, std::size_t N>
constexpr std::size_t array_size(T (&)[N]) noexcept {
    return N;
}

static void reparse_test(const whatwg::url& u1) {
    whatwg::url u2;

    // reparse must succeed
    assert(whatwg::success(u2.parse(u1.href(), nullptr)));

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
    const whatwg::url* pbase = ind < array_size(base_urls) ? &base_urls[ind] : nullptr;

    // skip first byte of data
    ++data; --size;

    // Parse input data against base URL
    whatwg::url::str_view_type inp{ data, size };
    try {
        whatwg::url u1{ inp, pbase };
        reparse_test(u1);
    }
    catch (whatwg::url_error&) {
        // invalid input
    }
    catch (std::exception&) {
        assert(false);
    }
    return 0;
}
