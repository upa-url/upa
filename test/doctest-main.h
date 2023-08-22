// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"
#include "url_cleanup.h"

// The main() entry point
// https://github.com/onqtam/doctest/blob/master/doc/markdown/main.md

int main(int argc, char** argv) {
    doctest::Context context;

    // apply command line
    context.applyCommandLine(argc, argv);

    // run test cases
    const int res = context.run();

    // Free memory
    upa::url_cleanup();

    return res;
}
