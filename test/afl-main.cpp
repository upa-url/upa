// Copyright 2016-2020 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

// American fuzzy lop (AFL) entry point

#include <iostream>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const char* data, std::size_t size);

int main() {
    std::vector<char> buff;
    buff.reserve(1024);

    // https://github.com/google/AFL/tree/master/llvm_mode
#ifdef __AFL_HAVE_MANUAL_CONTROL
    while (__AFL_LOOP(5000))
#endif
    {   // read all from std::cin
        buff.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());

        // test
        LLVMFuzzerTestOneInput(buff.data(), buff.size());
    }
    return 0;
}
