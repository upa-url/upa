// Copyright 2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#include "upa/public_suffix_list.h"

#define ANKERL_NANOBENCH_IMPLEMENT
#include "ankerl/nanobench.h"

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int bench_psl_list(const char* filename) {
    constexpr uint64_t min_iters = 128;
    std::vector<std::string> domain_list;

    std::ifstream finp(filename, std::ios_base::in | std::ios_base::binary);
    if (!finp) {
        std::cerr << "Can not open: " << filename << '\n';
        return 1;
    }

    std::string line;
    while (std::getline(finp, line)) {
        if (line.empty())
            continue;
        if (line.length() >= 2 && line[0] == '/' && line[1] == '/')
            continue;

        const auto isep = line.find(' ');
        domain_list.push_back(line.substr(0, isep));
    }

    const char* filename_psl = "psl/public_suffix_list.dat";

    upa::public_suffix_list ps_list;
    if (!ps_list.load(filename_psl)) {
        std::cerr << "Can not open: " << filename_psl << '\n';
        return 1;
    }

    ankerl::nanobench::Bench().minEpochIterations(min_iters).run("public_suffix_list",
        [&] {
            for (const auto& str_domain : domain_list) {
                std::string reg_domain = ps_list.get_suffix(str_domain, true);
                ankerl::nanobench::doNotOptimizeAway(reg_domain);
            }
        });

    return 0;
}

int main() {
    return bench_psl_list("psl/tests.txt");
}
