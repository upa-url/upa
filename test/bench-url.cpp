// Copyright 2023-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url.h"
#include "picojson_util.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#define ANKERL_NANOBENCH_IMPLEMENT
#include "ankerl/nanobench.h"

// -----------------------------------------------------------------------------
// Read samples from text file (URL in each line) and benchmark

int benchmark_txt(const char* file_name, uint64_t min_iters) {
    std::vector<std::string> url_strings;

    // Load URL samples
    std::cout << "Load URL samples from: " << file_name << '\n';
    std::ifstream finp(file_name);
    if (!finp.is_open()) {
        std::cout << "Failed to open " << file_name << '\n';
        return 2;
    }

    std::string line;
    while (std::getline(finp, line))
        url_strings.push_back(line);

    // Run benchmark

    ankerl::nanobench::Bench().minEpochIterations(min_iters).run("Upa url::parse", [&] {
        upa::url url;

        for (const auto& str_url : url_strings) {
            url.parse(str_url);

            ankerl::nanobench::doNotOptimizeAway(url);
        }
    });

    ankerl::nanobench::Bench().minEpochIterations(min_iters).run("Upa url::can_parse", [&] {
        for (const auto& str_url : url_strings) {
            bool ok = upa::url::can_parse(str_url);

            ankerl::nanobench::doNotOptimizeAway(ok);
        }
    });

    return 0;
}

// -----------------------------------------------------------------------------
// Read samples from urltestdata.json and benchmark

int benchmark_wpt(const char* file_name, uint64_t min_iters) {
    std::vector<std::pair<std::string, std::string>> url_samples;

    // Load URL samples
    json_util::root_array_context context{ [&](const picojson::value& item) {
        if (item.is<picojson::object>()) {
            try {
                const picojson::object& obj = item.get<picojson::object>();
                const auto input_val = obj.at("input");
                const auto base_val = obj.at("base");

                url_samples.emplace_back(
                    input_val.get<std::string>(),
                    base_val.is<picojson::null>() ? std::string{} : base_val.get<std::string>());
            }
            catch (const std::out_of_range& ex) {
                std::cout << "[ERR:invalid file]: " << ex.what() << std::endl;
                return false;
            }
        }
        return true;
    } };

    const int err = json_util::load_file(context, file_name, "Load URL samples from");
    if (err != 0)
        return err;

    // Run benchmark

    ankerl::nanobench::Bench().minEpochIterations(min_iters).run("Upa url::parse", [&] {
        upa::url url;
        upa::url url_base;

        for (const auto& url_strings : url_samples) {
            upa::url* ptr_base = nullptr;
            if (!url_strings.second.empty()) {
                if (!upa::success(url_base.parse(url_strings.second)))
                    continue; // invalid base
                ptr_base = &url_base;
            }
            url.parse(url_strings.first, ptr_base);

            ankerl::nanobench::doNotOptimizeAway(url);
        }
    });

    ankerl::nanobench::Bench().minEpochIterations(min_iters).run("Upa url::can_parse", [&] {
        for (const auto& url_strings : url_samples) {
            bool ok = url_strings.second.empty()
                ? upa::url::can_parse(url_strings.first)
                : upa::url::can_parse(url_strings.first, url_strings.second);

            ankerl::nanobench::doNotOptimizeAway(ok);
        }
    });

    return 0;
}

// -----------------------------------------------------------------------------

uint64_t get_positive_or_default(const char* str, uint64_t def)
{
    const uint64_t res = std::strtoull(str, nullptr, 10);
    if (res > 0)
        return res;
    return def;
}

int main(int argc, const char* argv[])
{
    constexpr uint64_t min_iters_def = 3;

    if (argc < 2) {
        std::cerr << "Usage: bench-url <file containing URLs> [<min iterations>]\n";
        return 1;
    }

    const std::filesystem::path file_name = argv[1];
    const uint64_t min_iters = argc > 2
        ? get_positive_or_default(argv[2], min_iters_def)
        : min_iters_def;

    if (file_name.extension() == ".json") {
        return benchmark_wpt(file_name.string().c_str(), min_iters);
    } else if (file_name.extension() == ".txt") {
        return benchmark_txt(file_name.string().c_str(), min_iters);
    }

    std::cerr << "File containing URLs should have .json or .txt extension.\n";
    return 1;
}
