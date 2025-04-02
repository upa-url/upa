// Copyright 2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_REGEX_ENGINE_STD_H
#define UPA_REGEX_ENGINE_STD_H

#include <cstddef>
#include <optional>
#include <regex>
#include <string>
#include <string_view>

namespace upa::pattern {

class regex_engine_std {
public:
    class result {
    public:
        std::size_t size() const {
            return arr_.size();
        }
        std::optional<std::string> get(std::size_t ind, std::string_view) const {
            const auto& res = arr_[ind];
            if (res.matched)
                return std::string{ res.first, res.second };
            return std::nullopt;
        }
    private:
        std::match_results<std::string_view::const_iterator> arr_;
        friend class regex_engine_std;
    };

    bool init(std::string_view regex_str, bool ignore_case) {
        // 4. If options's ignore case is true then set flags to "vi".
        // 5. Otherwise set flags to "v"
        // TODO: "v" flag
        const std::regex::flag_type flag = ignore_case
            ? (std::regex::ECMAScript | std::regex::icase)
            : std::regex::ECMAScript;
        try {
            re_.assign(regex_str.data(), regex_str.length(), flag);
            return true;
        }
        catch (const std::regex_error&) {
            return false;
        }
    }

    bool exec(std::string_view input, result& res) const {
        return std::regex_match(input.begin(), input.end(), res.arr_, re_);
    }
    bool test(std::string_view input) const {
        return std::regex_match(input.begin(), input.end(), re_);
    }

private:
    std::regex re_;
};

} // namespace upa::pattern

#endif // UPA_REGEX_ENGINE_STD_H
