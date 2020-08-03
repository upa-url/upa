// Relative URL parsing fuzzer

#include "url.h"
#include <cassert>
#include <type_traits>


// Base URLs
static const whatwg::url baseUrls[] = {
    whatwg::url("http://h/p?q#f"),      // 1
    whatwg::url("file://h/p?q#f"),      // 2
    whatwg::url("non-spec://h/p?q#f"),  // 3
    // with empty host
    whatwg::url("file:///p?q#f"),       // 4
    whatwg::url("non-spec:///p?q#f"),   // 5
    // with null host
    whatwg::url("non-spec:/p?q#f"),     // 6
    whatwg::url("non-spec:p?q#f")       // 7
};


template <typename T, std::size_t N>
constexpr std::size_t arraySize(T (&)[N]) noexcept {
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
// http://llvm.org/docs/LibFuzzer.html

extern "C" int LLVMFuzzerTestOneInput(const char* data, std::size_t size) {
    // Get base URL
    if (size < 1) return 0;
    // first byte (code) means what base URL to use:
    // '0' - no base URL
    // '1'... - base URL from baseUrls by index (code - '0' - 1)
    const auto code = data[0];
    if (code < '0') return 0;
    const std::size_t ind = code - '0';
    if (ind > arraySize(baseUrls)) return 0;
    const whatwg::url* pbase = ind > 0 ? &baseUrls[ind - 1] : nullptr;

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
