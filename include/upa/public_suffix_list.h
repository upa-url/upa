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

/// @brief Get label position in hostname by it's index
///
/// The label separator can be any of the following characters: U+002E (.),
/// U+3002, U+FF0E, U+FF61 (they are mapped to U+002E (.) by IDNA).
///
/// @param[in] str_host hostname string
/// @param[in] index zero-based label index starting from the leftmost label
/// @return label position in the @p str_host string
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

/// @brief The Public Suffix List class
///
/// It contains functions for loading Public Suffix List, getting public suffix
/// or registrable domain of hostname or getting information about them.
///
/// It implements the [Public Suffix List](https://publicsuffix.org/) algorithm defined
/// here: https://github.com/publicsuffix/list/wiki/Format#formal-algorithm.
/// It also implements the URL standard
/// [public suffix](https://url.spec.whatwg.org/#host-public-suffix) and
/// [registrable domain](https://url.spec.whatwg.org/#host-registrable-domain)
/// algorithms.
///
/// The Public Suffix List file `public_suffix_list.dat` is required and can be obtained from
/// https://publicsuffix.org/list/ or https://github.com/publicsuffix/list.
///
/// @par Example
/// @code
/// #include "upa/public_suffix_list.h"
/// #include <iostream>
///
/// int main() {
///     upa::public_suffix_list psl;
///     if (psl.load("public_suffix_list.dat")) {
///         auto input = "upa-url.github.io";
///         auto suffix = psl.get_suffix_view(input);
///         std::cout << "Public suffix of " << input << " is " << suffix << '\n';
///     }
/// }
/// @endcode
class public_suffix_list {
    // label_item::code values
    static constexpr std::uint8_t DIFF_MASK = 3;
    static constexpr std::uint8_t IS_ICANN = 4;
    static constexpr std::uint8_t IS_PRIVATE = 8;
public:
    /// @brief Options for public_suffix_list::get_suffix, public_suffix_list::get_suffix_info
    /// and public_suffix_list::get_suffix_view functions
    enum class option {
        PUBLIC_SUFFIX = 0,      ///< to get public suffix
        REGISTRABLE_DOMAIN = 1, ///< to get registrable domain
        ALLOW_TRAILING_DOT = 2  ///< allow trailing dot in hostname input to get result as
                                ///< defined in the URL standard; see:
                                ///< https://url.spec.whatwg.org/#host-public-suffix
    };
#ifdef __cpp_using_enum
    using enum option; // C++20
#else
    static constexpr auto PUBLIC_SUFFIX = option::PUBLIC_SUFFIX;
    static constexpr auto REGISTRABLE_DOMAIN = option::REGISTRABLE_DOMAIN;
    static constexpr auto ALLOW_TRAILING_DOT = option::ALLOW_TRAILING_DOT;
#endif

    /// @brief Return value type of the public_suffix_list::get_suffix_info function
    struct result {
        /// @return true if public suffix or registrable domain is not null
        constexpr operator bool() const noexcept {
            return first_label_ind != static_cast<std::size_t>(-1);
        }
        /// @return true if public suffix is in ICANN section
        constexpr bool is_icann() const noexcept {
            return (code_ & IS_ICANN) != 0;
        }
        /// @return true if public suffix is in PRIVATE section
        constexpr bool is_private() const noexcept {
            return (code_ & IS_PRIVATE) != 0;
        }
        /// @return true if hostname matches wildcard rule
        constexpr bool wildcard_rule() const noexcept {
            return (code_ & DIFF_MASK) == 3;
        }
        /// First label index of public suffix or registrable domain in hostname
        std::size_t first_label_ind = static_cast<std::size_t>(-1);
        /// First label position of public suffix or registrable domain in hostname
        std::size_t first_label_pos = static_cast<std::size_t>(-1);
        std::uint8_t code_ = 0;
    };

    /// @brief Load Public Suffix List from file
    ///
    /// The Public Suffix List file `public_suffix_list.dat` can be obtained from
    /// https://publicsuffix.org/list/ or https://github.com/publicsuffix/list
    ///
    /// @param[in] filename the path to the Public Suffix List file
    /// @return true if the Public Suffix List was loaded successfully
    bool load(const std::filesystem::path& filename) {
        std::ifstream finp(filename, std::ios_base::in | std::ios_base::binary);
        return finp && load(finp);
    }
    /// @brief Load Public Suffix List from stream
    ///
    /// @param[in] input_stream input stream of the Public Suffix List file
    /// @return true if the Public Suffix List was loaded successfully
    bool load(std::istream& input_stream);

    // Push interface to load Public Suffix List

    /// @brief Push context for public_suffix_list::push_line, public_suffix_list::push
    /// and public_suffix_list::finalize functions
    struct push_context {
        std::string remaining{};
        std::uint8_t code_flags = 0;
    };

    /// @brief Push a line of the Public Suffix List file
    ///
    /// There is no need to call public_suffix_list::finalize after pushing all lines.
    ///
    /// @param[in] ctx push context
    /// @param[in] line text line to push
    void push_line(push_context& ctx, std::string_view line);

    /// @brief Push a chunk of the Public Suffix List file
    ///
    /// Useful when the Public Suffix List is downloaded chunk by chunk over the network.
    /// public_suffix_list::finalize must be called after all chunks have been processed.
    ///
    /// @param[in] ctx push context
    /// @param[in] buff chunk content
    void push(push_context& ctx, std::string_view buff);

    /// @brief Finalizes when all chunks are processed
    ///
    /// Must be called after all chunks have been processed by the
    /// public_suffix_list::push function
    ///
    /// @param[in] ctx push context
    /// @return true if the Public Suffix List was loaded successfully
    bool finalize(push_context& ctx);

    // Get public suffix or registrable domain

    /// @brief Get public suffix or registrable domain
    ///
    /// @param[in] str_host hostname string
    /// @param[in] opt options
    /// @return public suffix or registrable domain (depends on @p opt value)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    std::string get_suffix(StrT&& str_host, option opt = PUBLIC_SUFFIX) const {
        try {
            return std::string{ get_suffix_view(
                upa::url_host{ std::forward<StrT>(str_host) },
                opt) };
        }
        catch (const upa::url_error&) {
            return {};
        }
    }

    /// @brief Get public suffix or registrable domain information
    ///
    /// Returns information about the public suffix or registrable domain in a
    /// hostname returned by url::hostname() const.
    ///
    /// @param[in] url the @ref url object
    /// @param[in] opt options
    /// @return information in the public_suffix_list::result type struct
    result get_suffix_info(const url& url, option opt = PUBLIC_SUFFIX) const {
        if (url.host_type() == HostType::Domain)
            return get_host_suffix_info(url.hostname(), opt);
        return {};
    }

    /// @brief Get public suffix or registrable domain information
    ///
    /// Returns information about the public suffix or registrable domain of the
    /// @p host.
    ///
    /// @param[in] host the url_host object
    /// @param[in] opt options
    /// @return information in the public_suffix_list::result type struct
    result get_suffix_info(const url_host& host, option opt = PUBLIC_SUFFIX) const {
        if (host.type() == HostType::Domain)
            return get_host_suffix_info(host.name(), opt);
        return {};
    }

    /// @brief Get public suffix or registrable domain information
    ///
    /// Returns information about the public suffix or registrable domain of the
    /// @p str_host string.
    ///
    /// @param[in] str_host hostname string
    /// @param[in] opt options
    /// @return information in the public_suffix_list::result type struct
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    result get_suffix_info(StrT&& str_host, option opt = PUBLIC_SUFFIX) const {
        try {
            auto res = get_suffix_info(upa::url_host{ std::forward<StrT>(str_host) }, opt);
            res.first_label_pos = get_label_pos_by_index(str_host, res.first_label_ind);
            return res;
        }
        catch (const upa::url_error&) {
            return {};
        }
    }

    /// @brief Get public suffix or registrable domain as string view
    ///
    /// Returns the public suffix or registrable domain of a hostname returned
    /// by url::hostname() const.
    ///
    /// @param[in] url the @ref url object
    /// @param[in] opt options
    /// @return public suffix or registrable domain (depends on @p opt value)
    std::string_view get_suffix_view(const url& url, option opt = PUBLIC_SUFFIX) const {
        if (url.host_type() == HostType::Domain)
            return get_host_suffix_view(url.hostname(), opt);
        return {};
    }

    /// @brief Get public suffix or registrable domain as string view
    ///
    /// Returns the public suffix or registrable domain of the @p host.
    ///
    /// @param[in] host the url_host object
    /// @param[in] opt options
    /// @return public suffix or registrable domain (depends on @p opt value)
    std::string_view get_suffix_view(const url_host& host, option opt = PUBLIC_SUFFIX) const {
        if (host.type() == HostType::Domain)
            return get_host_suffix_view(host.name(), opt);
        return {};
    }

    /// @brief Get public suffix or registrable domain as string view
    ///
    /// Returns the public suffix or registrable domain of the @p str_host string.
    ///
    /// @param[in] str_host hostname string
    /// @param[in] opt options
    /// @return public suffix or registrable domain (depends on @p opt value)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    auto get_suffix_view(StrT&& str_host, option opt = PUBLIC_SUFFIX) const
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

    /// @brief Compares @c *this with @p other
    /// @param[in] other the public_suffix_list to compare with
    /// @return true if both lists are equal
    bool operator==(const public_suffix_list& other) const {
        return root_ == other.root_;
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

        bool operator==(const label_item& b) const {
            return code == b.code && (
                (!children && !b.children) ||
                (children && b.children && *children == *b.children));
        }
    };

    label_item root_;
};

namespace idna {

template<>
struct enable_bitmask_operators<upa::public_suffix_list::option>
    : public std::true_type {};

} // namespace idna
} // namespace upa

#endif // UPA_PUBLIC_SUFFIX_LIST_H
