// Copyright 2016-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

// The main() entry point
// https://github.com/doctest/doctest/blob/master/doc/markdown/main.md

int main(int argc, char** argv) {
    doctest::Context context;

    // apply command line
    context.applyCommandLine(argc, argv);

    // run test cases
    return context.run();
}
