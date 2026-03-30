// Copyright 2026 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#include "upa/urlpattern.h"
#ifdef UPA_TEST_WITH_STD_REGEX
# include "upa/regex_engine_std.h"
#else
# include "upa/regex_engine_srell.h"
#endif

#include <fuzzer/FuzzedDataProvider.h>

#include <cassert>

#ifdef UPA_TEST_WITH_STD_REGEX
using urlpattern = upa::urlpattern<upa::regex_engine_std>;
#else
using urlpattern = upa::urlpattern<upa::regex_engine_srell>;
#endif

// Use libFuzzer interface
// https://llvm.org/docs/LibFuzzer.html

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    FuzzedDataProvider fdp(data, size);

    auto urlp_input = fdp.ConsumeRandomLengthString();
    auto exec_input = fdp.ConsumeRemainingBytesAsString();

    try {
        urlpattern urlp{ urlp_input };

        auto res1 = urlp.exec(exec_input);
        auto res2 = urlp.test(exec_input);
        assert(res1.has_value() == res2);
    }
    catch (upa::urlpattern_error&) {
        // invalid urlp_input
    }
    catch (std::exception&) {
        assert(false);
    }
    return 0;
}
