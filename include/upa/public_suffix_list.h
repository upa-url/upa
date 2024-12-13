// Copyright 2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_PUBLIC_SUFFIX_LIST_H
#define UPA_PUBLIC_SUFFIX_LIST_H

#include "url.h"
#include <cstddef>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace upa {

class public_suffix_list {
public:
    // Load public suffix list from file
    template <typename CharT>
    bool load(const CharT* filename) {
        std::ifstream finp(filename, std::ios_base::in | std::ios_base::binary);
        return finp && load(finp);
    }
    bool load(std::istream& input_stream);

    // Push interface to load public suffix list 
    struct push_context {
        std::string remaining{};
        std::uint8_t code_flags = 0;
    };
    void push_line(push_context& ctx, std::string_view line);
    void push(push_context& ctx, std::string_view buff);
    bool finalize(push_context& ctx);

    // Get public suffix or registrable domain
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    std::string get_suffix(StrT&& str_host, bool reg_domain) const {
        try {
            return get_host_suffix(upa::url_host{ std::forward<StrT>(str_host) }.to_string(), reg_domain);
        }
        catch (const upa::url_error&) {
            return {};
        }
    }

    std::string get_suffix(const url& url, bool reg_domain) const {
        return get_host_suffix(url.hostname(), reg_domain);
    }

private:
    std::string get_host_suffix(std::string_view hostname, bool reg_domain) const;

private:
    struct label_item {
#ifdef __cpp_lib_generic_unordered_lookup
        // https://en.cppreference.com/w/cpp/container/unordered_map/find
        struct string_hash {
            using hash_type = std::hash<std::string_view>;
            using is_transparent = void;

            std::size_t operator()(const char* str) const { return hash_type{}(str); }
            std::size_t operator()(std::string_view str) const { return hash_type{}(str); }
            std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
        };
        using map_type = std::unordered_map<std::string, label_item, string_hash, std::equal_to<>>;
#else
        using map_type = std::unordered_map<std::string, label_item>;
#endif

        std::uint8_t code = 0;
        std::unique_ptr<map_type> children;

        static constexpr std::uint8_t DIFF_MASK = 3;
        static constexpr std::uint8_t IS_ICANN = 4;
        static constexpr std::uint8_t IS_PRIVATE = 8;
    };

    label_item root_;
};

} // namespace upa

#endif // UPA_PUBLIC_SUFFIX_LIST_H
