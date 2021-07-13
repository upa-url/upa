// Copyright 2016-2021 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

// American fuzzy lop (AFL) entry point

#include <iostream>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const char* data, std::size_t size);

#if defined(__AFL_FUZZ_TESTCASE_BUF)

// AFL shared memory fuzzing (afl++ 2.66c and later)
// https://github.com/AFLplusplus/AFLplusplus/blob/stable/instrumentation/README.persistent_mode.md
// https://github.com/AFLplusplus/AFLplusplus/blob/stable/utils/persistent_mode/persistent_demo_new.c

#include <unistd.h>

__AFL_FUZZ_INIT();

int main() {
    __AFL_INIT();

    // must be after __AFL_INIT:
    const char* buf = reinterpret_cast<const char*>(__AFL_FUZZ_TESTCASE_BUF);

    while (__AFL_LOOP(5000)) {
        const std::size_t len = __AFL_FUZZ_TESTCASE_LEN;

        // test
        LLVMFuzzerTestOneInput(buf, len);
    }
    return 0;
}

#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
// POSIX, see: https://stackoverflow.com/a/11351171

// AFL doesn't work well with buffered i/o. C++ standard library has only
// buffered i/o functions. So here is used unbuffered POSIX read function.
// See "STEP 2" here:
// https://github.com/google/AFL/blob/master/experimental/persistent_demo/persistent_demo.c

#include <unistd.h>

int main() {
    std::vector<char> buff;
    buff.resize(1024 * 1024);

#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();

    while (__AFL_LOOP(5000))
#endif
    {   // read all from stadard input
        const std::size_t len = read(0, buff.data(), buff.size());

        // test
        LLVMFuzzerTestOneInput(buff.data(), len);
    }
    return 0;
}

#else

int main() {
    std::vector<char> buff;
    buff.reserve(1024);

    // https://github.com/google/AFL/tree/master/llvm_mode
#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();

    while (__AFL_LOOP(5000))
#endif
    {   // read all from std::cin
        buff.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());

        // test
        LLVMFuzzerTestOneInput(buff.data(), buff.size());
    }
    return 0;
}

#endif
