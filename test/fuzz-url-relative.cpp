// Relative URL parsing fuzzer

#include "url.h"
#include <cassert>
#include <type_traits>


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
    static const whatwg::url baseUrls[] = {
        whatwg::url("http://h/p?q#f"),
        whatwg::url("file://h/p?q#f"),
        whatwg::url("non-spec://h/p?q#f")
    };

    if (size < 1) return 0;
    // first byte - index in the base URLs array
    if (data[0] < '0') return 0;
    const std::size_t ind = data[0] - '0';
    if (ind >= arraySize(baseUrls)) return 0;
    const auto& base = baseUrls[ind];
    // skip first byte of data
    ++data; --size;

    whatwg::url::str_view_type inp{ data, size };
    try {
        whatwg::url u1{ inp, base };
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
