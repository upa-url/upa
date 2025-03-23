// Copyright 2024-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#include "ddt/DataDrivenTest.hpp"
#include "upa/public_suffix_list.h"

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

// Global public suffix list
upa::public_suffix_list ps_list;

std::string ascii_lower(std::string_view inp) {
    std::string str;
    for (auto c : inp)
        str += upa::util::ascii_to_lower_char(c);
    return str;
}

int test_public_suffix_list(const std::filesystem::path& filename) {
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
        const std::string input = line.substr(0, isep);
        const std::string expected = line.substr(isep + 1); // skip ' '

        ddt.test_case(line, [&](DataDrivenTest::TestCase& tc) {
            // Tests expect lower case (ASCII) output
            std::string output = ascii_lower(ps_list.get_suffix_view(input,
                upa::public_suffix_list::option::registrable_domain));
            if (output.empty())
                output = "null";

            tc.assert_equal(expected, output, "get_suffix_view");
        });
    }

    return ddt.result();
}

int test_whatwg_public_suffix_list(const std::filesystem::path& filename) {
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

        const auto isep1 = line.find(' ');
        if (isep1 == std::string::npos) {
            std::cerr << "INVALID LINE: " << line << '\n';
            continue;
        }
        const auto isep2 = line.find(' ', isep1 + 1);
        if (isep2 == std::string::npos) {
            std::cerr << "INVALID LINE: " << line << '\n';
            continue;
        }

        const std::string input = line.substr(0, isep1);
        const std::string expected_suffix = line.substr(isep1 + 1, isep2 - (isep1 + 1)); // skip ' '
        const std::string expected_domain = line.substr(isep2 + 1); // skip ' '

        ddt.test_case(line, [&](DataDrivenTest::TestCase& tc) {
            std::string output_suffix = ps_list.get_suffix(input,
                upa::public_suffix_list::option::allow_trailing_dot);
            if (output_suffix.empty())
                output_suffix = "null";
            tc.assert_equal(expected_suffix, output_suffix, "get_suffix");

            std::string output_domain = ps_list.get_suffix(input,
                upa::public_suffix_list::option::allow_trailing_dot |
                upa::public_suffix_list::option::registrable_domain);
            if (output_domain.empty())
                output_domain = "null";
            tc.assert_equal(expected_suffix, output_suffix,
                "get_suffix (option::registrable_domain)");
        });
    }

    return ddt.result();
}

TEST_CASE("get_label_pos_by_index") {
    CHECK(upa::get_label_pos_by_index("b.a", 2) == 3);
}

TEST_SUITE("public_suffix_list::get_suffix") {
    TEST_CASE("input with registrable domain") {
        std::string input = "example.com";
        std::string_view expected = input;
        std::string output = ps_list.get_suffix(input,
            upa::public_suffix_list::option::registrable_domain);
        CHECK_MESSAGE(output == expected, "input: ", input);
    }
    TEST_CASE("input with invalid host") {
        std::string input = "<>.com";
        std::string_view expected = "";
        std::string output = ps_list.get_suffix(input,
            upa::public_suffix_list::option::registrable_domain);
        CHECK_MESSAGE(output == expected, "input: ", input);
    }
}

TEST_SUITE("public_suffix_list::get_suffix_info") {
    TEST_CASE("url with registrable domain") {
        upa::url input{ "http://EXAMPLE.COM" };
        const auto output = ps_list.get_suffix_info(input,
            upa::public_suffix_list::option::registrable_domain);
        INFO("input (url): ", input.href());
        REQUIRE(static_cast<bool>(output));
        CHECK(output.first_label_pos == 0);
        CHECK(output.first_label_ind == 0);
        CHECK(output.is_icann());
        CHECK_FALSE(output.is_private());
        CHECK_FALSE(output.wildcard_rule());
    }
    TEST_CASE("url with non-registrable domain") {
        upa::url input{ "http://com" };
        const auto output = ps_list.get_suffix_info(input,
            upa::public_suffix_list::option::registrable_domain);
        INFO("input (url): ", input.href());
        CHECK_FALSE(static_cast<bool>(output));
    }
    TEST_CASE("url with IPv4 address") {
        upa::url input{ "http://127.0.0.1" };
        const auto output = ps_list.get_suffix_info(input);
        INFO("input (url): ", input.href());
        CHECK_FALSE(static_cast<bool>(output));
    }

    TEST_CASE("url_host with registrable domain") {
        upa::url_host input{ "upa-url.github.io" };
        const auto output = ps_list.get_suffix_info(input,
            upa::public_suffix_list::option::registrable_domain);
        INFO("input (url_host): ", input.name());
        REQUIRE(static_cast<bool>(output));
        CHECK(output.first_label_pos == 0);
        CHECK(output.first_label_ind == 0);
        CHECK_FALSE(output.is_icann());
        CHECK(output.is_private());
        CHECK_FALSE(output.wildcard_rule());
    }
    TEST_CASE("url_host with non-registrable domain") {
        upa::url_host input{ "github.io" };
        const auto output = ps_list.get_suffix_info(input,
            upa::public_suffix_list::option::registrable_domain);
        INFO("input (url_host): ", input.name());
        CHECK_FALSE(static_cast<bool>(output));
    }
    TEST_CASE("url_host with IPv4 address") {
        upa::url_host input{ "127.0.0.1" };
        const auto output = ps_list.get_suffix_info(input);
        INFO("input (url_host): ", input.name());
        CHECK_FALSE(static_cast<bool>(output));
    }

    TEST_CASE("registrable domain") {
        std::string input = "a.b.c.hosted.app";
        const auto output = ps_list.get_suffix_info(input,
            upa::public_suffix_list::option::registrable_domain);
        INFO("input: ", input);
        // Rule: *.hosted.app => registrable domain: b.c.hosted.app
        REQUIRE(static_cast<bool>(output));
        CHECK(output.first_label_pos == 2);
        CHECK(output.first_label_ind == 1);
        CHECK_FALSE(output.is_icann());
        CHECK(output.is_private());
        CHECK(output.wildcard_rule());
    }
    TEST_CASE("invalid domain") {
        std::string input = "<>.com";
        const auto output = ps_list.get_suffix_info(input,
            upa::public_suffix_list::option::registrable_domain);
        INFO("input: ", input);
        REQUIRE_FALSE(static_cast<bool>(output));
    }
}

TEST_SUITE("public_suffix_list::get_suffix_view") {
    TEST_CASE("url with registrable domain") {
        upa::url input{ "http://EXAMPLE.ORG" };
        std::string_view expected = "example.org";
        std::string_view output = ps_list.get_suffix_view(input,
            upa::public_suffix_list::option::registrable_domain);
        CHECK_MESSAGE(output == expected, "input (url): ", input.href());
    }
    TEST_CASE("url with non-registrable domain") {
        upa::url input{ "http://org" };
        std::string_view expected = "";
        std::string_view output = ps_list.get_suffix_view(input,
            upa::public_suffix_list::option::registrable_domain);
        CHECK_MESSAGE(output == expected, "input (url): ", input.href());
    }
    TEST_CASE("url with IPv6 address") {
        upa::url input{ "http://[::1]" };
        const auto output = ps_list.get_suffix_view(input);
        CHECK_MESSAGE(output.empty(), "input (url): ", input.href());
    }

    TEST_CASE("invalid domain") {
        std::string input = "<>.com";
        const auto output = ps_list.get_suffix_view(input);
        CHECK_MESSAGE(output.empty(), "input: ", input);
    }
}

TEST_SUITE("public_suffix_list push interface") {
    TEST_CASE("public_suffix_list::push") {
        const std::filesystem::path filename{ "psl/public_suffix_list.dat" };
        std::ifstream finp(filename, std::ios_base::in | std::ios_base::binary);
        REQUIRE(finp);

        upa::public_suffix_list psl;
        upa::public_suffix_list::push_context ctx;
        char buffer[64];
        while (finp) {
            finp.read(buffer, sizeof(buffer));
            psl.push(ctx, { buffer, static_cast<std::size_t>(finp.gcount()) });
        }
        REQUIRE(psl.finalize(ctx));

        CHECK(psl == ps_list);
    }
    TEST_CASE("public_suffix_list::finalize") {
        upa::public_suffix_list psl;
        upa::public_suffix_list::push_context ctx;
        // Since PSL is empty, the "*" rule applies
        CHECK(psl.get_suffix("upa-url.github.io") == "io");
        // Push text without EOL
        psl.push(ctx, "github.io");
        CHECK(psl.finalize(ctx));
        CHECK(psl.get_suffix("upa-url.github.io") == "github.io");
    }
}

int test_public_suffix_list_functions(int argc, char** argv) {
    std::cout << "========== Test public_suffix_list functions ==========\n";

    doctest::Context context;

    // apply command line
    context.applyCommandLine(argc, argv);

    // run test cases
    return context.run();
}

int main(int argc, char** argv) {
    const std::filesystem::path filename_psl{ "psl/public_suffix_list.dat" };

    if (!ps_list.load(filename_psl)) {
        std::cerr << "Can not open: " << filename_psl << '\n';
        return 1;
    }

    int err = 0;
    err |= test_public_suffix_list("psl/tests.txt");
    err |= test_public_suffix_list("data/my-psl-tests.txt");
    err |= test_whatwg_public_suffix_list("data/whatwg-psl-tests.txt");
    err |= test_public_suffix_list_functions(argc, argv);

    return err;
}
