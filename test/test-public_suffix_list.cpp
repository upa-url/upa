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


std::string ascii_lower(std::string_view inp) {
    std::string str;
    for (auto c : inp)
        str += upa::util::ascii_to_lower_char(c);
    return str;
}

int test_public_suffix_list(const upa::public_suffix_list& ps_list, const std::filesystem::path& filename) {
    // Open tests file
    std::cout << "========== " << filename << " ==========\n";
    std::ifstream finp(filename, std::ios_base::in);
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
            // Tests expect lower case (ASCII) output
            std::string output = ascii_lower(ps_list.get_suffix_view(domain, true));
            if (output.empty())
                output = "null";

            tc.assert_equal(expected, output, "get_suffix_view");
        });
    }

    return ddt.result();
}

int test_public_suffix_list_functions(const upa::public_suffix_list& ps_list) {
    DataDrivenTest ddt;

    std::cout << "========== Test public_suffix_list functions ==========\n";

    ddt.test_case("public_suffix_list::get_suffix", [&](DataDrivenTest::TestCase& tc) {
        std::string input = "example.com";
        std::string_view expected = input;
        std::string output = ps_list.get_suffix(input, true);
        tc.assert_equal(expected, output, "get_suffix(\"example.com\")");

        input = "<>.com"; // invalid host
        expected = "";
        output = ps_list.get_suffix(input, true);
        tc.assert_equal(expected, output, "get_suffix(\"<>.com\")");
    });

    ddt.test_case("public_suffix_list::get_suffix_view", [&](DataDrivenTest::TestCase& tc) {
        upa::url input{ "http://EXAMPLE.COM" };
        std::string_view expected = "example.com";
        std::string_view output = ps_list.get_suffix_view(input, true);
        tc.assert_equal(expected, output, "get_suffix_view(url(\"http://EXAMPLE.COM\"))");
    });

    return ddt.result();
}

int main() {
    const std::filesystem::path filename_psl{ "psl/public_suffix_list.dat" };

    upa::public_suffix_list ps_list;
    if (!ps_list.load(filename_psl)) {
        std::cerr << "Can not open: " << filename_psl << '\n';
        return 1;
    }

    int err = 0;
    err |= test_public_suffix_list(ps_list, "psl/tests.txt");
    err |= test_public_suffix_list(ps_list, "data/my-psl-tests.txt");
    err |= test_public_suffix_list_functions(ps_list);

    return err;
}
