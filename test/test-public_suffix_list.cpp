// Copyright 2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#include "ddt/DataDrivenTest.hpp"
#include "upa/public_suffix_list.h"

#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

int test_public_suffix_list(const upa::public_suffix_list& ps_list, const std::filesystem::path& filename) {
    // Open tests file
    std::cout << "========== " << filename << " ==========\n";
    std::ifstream finp(filename, std::ios_base::in | std::ios_base::binary);
    if (!finp) {
        std::cerr << "Can not open: " << filename << '\n';
        return 1;
    }

    DataDrivenTest ddt;

    std::string line;
    while (std::getline(finp, line)) {
        if (line.empty())
            continue;
        if (line.length() >= 2 && line[0] == '/' && line[1] == '/')
            continue;

        const auto isep = line.find(' ');
        if (isep == std::string::npos) {
            std::cerr << "INVALID LINE: " << line << '\n';
            continue;
        }
        const std::string domain = line.substr(0, isep);
        const std::string expected = line.substr(isep + 1); // skip ' '

        ddt.test_case(line, [&](DataDrivenTest::TestCase& tc) {
            std::string output = ps_list.get_suffix(domain, true);
            if (output.empty())
                output = "null";

            tc.assert_equal(expected, output, "get_suffix");
        });
    }

    return ddt.result();
}

int main() {
    const std::filesystem::path filename_psl{ "psl/public_suffix_list.dat" };

    upa::public_suffix_list ps_list;
    if (!ps_list.load(filename_psl)) {
        std::cerr << "Can not open: " << filename_psl << '\n';
        return 1;
    }

    return test_public_suffix_list(ps_list, "psl/tests.txt");
}
