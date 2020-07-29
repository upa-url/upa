// URL parsing fuzzer

#include "url.h"
#include <cassert>


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
    whatwg::url::str_view_type inp{ data, size };

    try {
        whatwg::url u1{ inp };
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
