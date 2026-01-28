// Copyright 2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_REGEX_ENGINE_SRELL_H
#define UPA_REGEX_ENGINE_SRELL_H

#include "srell/srell.hpp"
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace upa {

class regex_engine_srell {
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
        srell::match_results<std::string_view::const_iterator> arr_;
        friend class regex_engine_srell;
    };

    bool init(std::string_view regex_str, bool ignore_case) {
        // 4. If options's ignore case is true then set flags to "vi".
        // 5. Otherwise set flags to "v"
        const auto commonflags = srell::regex::ECMAScript | srell::regex::vmode |
            srell::regex::quiet; // do not throw a srell::regex_error on errors
        const auto flag = ignore_case
            ? (commonflags | srell::regex::icase)
            : commonflags;
        return re_.assign(regex_str.data(), regex_str.length(), flag).ecode() == 0;
    }

    bool exec(std::string_view input, result& res) const {
        return srell::regex_match(input.begin(), input.end(), res.arr_, re_);
    }
    bool test(std::string_view input) const {
        return srell::regex_match(input.begin(), input.end(), re_);
    }

private:
    srell::regex re_;
};

} // namespace upa

#endif // UPA_REGEX_ENGINE_SRELL_H
