// Copyright 2024-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_PUBLIC_SUFFIX_LIST_H
#define UPA_PUBLIC_SUFFIX_LIST_H

#include "url.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace upa {

// get label position in hostname by it's index

template <class StrT, enable_if_str_arg_t<StrT> = 0>
constexpr std::size_t get_label_pos_by_index(StrT&& str_host, std::size_t index) {
    const auto inp = make_str_arg(std::forward<StrT>(str_host));
    const auto* first = inp.begin();
    const auto* last = inp.end();
    const auto* ptr = first;
    for (; index > 0; --index) {
        while (true) {
            if (ptr == last)
                return static_cast<std::size_t>(ptr - first);
            const auto c = util::to_unsigned(*ptr);
            if (c < 0x80) {
                ++ptr; // advance to next char
                if (c == '.')
                    break;
            } else {
                const auto cp = url_utf::read_utf_char(ptr, last).value;
                // These code points are mapped to '.' in the IDNA mapping table
                if (cp == 0x3002 || cp == 0xFF0E || cp == 0xFF61)
                    break;
            }
        }
    }
    return static_cast<std::size_t>(ptr - first);
}

// public_suffix_list class

class public_suffix_list {
    // label_item::code values
    static constexpr std::uint8_t DIFF_MASK = 3;
    static constexpr std::uint8_t IS_ICANN = 4;
    static constexpr std::uint8_t IS_PRIVATE = 8;
public:
    enum class option {
        PUBLIC_SUFFIX = 0,
        REGISTRABLE_DOMAIN = 1,
        ALLOW_TRAILING_DOT = 2
    };
#ifdef __cpp_using_enum
    using enum option; // C++20
#else
    static constexpr auto PUBLIC_SUFFIX = option::PUBLIC_SUFFIX;
    static constexpr auto REGISTRABLE_DOMAIN = option::REGISTRABLE_DOMAIN;
    static constexpr auto ALLOW_TRAILING_DOT = option::ALLOW_TRAILING_DOT;
#endif

    struct result {
        constexpr operator bool() const noexcept {
            return first_label_ind != static_cast<std::size_t>(-1);
        }
        constexpr bool is_icann() const noexcept {
            return (code_ & IS_ICANN) != 0;
        }
        constexpr bool is_private() const noexcept {
            return (code_ & IS_PRIVATE) != 0;
        }
        constexpr bool wildcard_rule() const noexcept {
            return (code_ & DIFF_MASK) == 3;
        }
        std::size_t first_label_ind = static_cast<std::size_t>(-1);
        std::size_t first_label_pos = static_cast<std::size_t>(-1);
        std::uint8_t code_ = 0;
    };

    // Load public suffix list from file
    bool load(const std::filesystem::path& filename) {
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
    std::string get_suffix(StrT&& str_host, option opt) const {
        try {
            return std::string{ get_suffix_view(
                upa::url_host{ std::forward<StrT>(str_host) },
                opt) };
        }
        catch (const upa::url_error&) {
            return {};
        }
    }

    result get_suffix_info(const url& url, option opt) const {
        if (url.host_type() == HostType::Domain)
            return get_host_suffix_info(url.hostname(), opt);
        return {};
    }

    result get_suffix_info(const url_host& host, option opt) const {
        if (host.type() == HostType::Domain)
            return get_host_suffix_info(host.name(), opt);
        return {};
    }

    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    result get_suffix_info(StrT&& str_host, option opt) const {
        try {
            auto res = get_suffix_info(upa::url_host{ std::forward<StrT>(str_host) }, opt);
            res.first_label_pos = get_label_pos_by_index(str_host, res.first_label_ind);
            return res;
        }
        catch (const upa::url_error&) {
            return {};
        }
    }

    std::string_view get_suffix_view(const url& url, option opt) const {
        if (url.host_type() == HostType::Domain)
            return get_host_suffix_view(url.hostname(), opt);
        return {};
    }

    std::string_view get_suffix_view(const url_host& host, option opt) const {
        if (host.type() == HostType::Domain)
            return get_host_suffix_view(host.name(), opt);
        return {};
    }

    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    auto get_suffix_view(StrT&& str_host, option opt) const
        -> std::basic_string_view<typename str_arg<str_arg_char_t<StrT>>::value_type> {
        try {
            const auto arg = make_str_arg(std::forward<StrT>(str_host));
            const upa::url_host host{ arg };
            if (host.type() == HostType::Domain) {
                const auto res = get_host_suffix_info(host.name(), opt);
                if (res) {
                    const auto pos = get_label_pos_by_index(arg, res.first_label_ind);
                    return { arg.data() + pos, arg.size() - pos };
                }
            }
        }
        catch (const upa::url_error&) {}
        return {};
    }

private:
    result get_host_suffix_info(std::string_view hostname, option opt) const;

    std::string_view get_host_suffix_view(std::string_view hostname, option opt) const {
        const auto res = get_host_suffix_info(hostname, opt);
        if (res)
            return hostname.substr(res.first_label_pos);
        return {};
    }

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
    };

    label_item root_;
};

} // namespace upa

template<>
struct enable_bitmask_operators<upa::public_suffix_list::option> {
    static const bool enable = true;
};

#endif // UPA_PUBLIC_SUFFIX_LIST_H
