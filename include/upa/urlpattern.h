// Copyright 2023-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_URLPATTERN_H
#define UPA_URLPATTERN_H

#include "unicode_id.h"
#include "url.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility> // pair
#include <variant>
#include <vector>

namespace upa::pattern {

using namespace std::string_view_literals;

// Scheme info

inline bool is_special_scheme(std::string_view scheme) {
    const auto* scheme_inf = detail::get_scheme_info(scheme);
    return scheme_inf != nullptr && scheme_inf->is_special;
}

inline bool is_special_scheme_default_port(std::string_view scheme, std::string_view port) {
    // scheme is special?
    const auto* scheme_inf = detail::get_scheme_info(scheme);
    if (scheme_inf != nullptr && scheme_inf->is_special && scheme_inf->default_port >= 0) {
        // port is valid and is the default scheme port?
        const auto* first = port.data();
        const auto* last = port.data() + port.length();
        std::uint16_t nport = 0;
        const auto r = std::from_chars(first, last, nport);
        return r.ec == std::errc() && r.ptr == last && nport == scheme_inf->default_port;
    }
    return false;
}

// TODO: make public in URL library:
// * make the list of special schemes public (see: protocol_component_matches_special_scheme)

// Parse URL against base URL

inline upa::url parse_url_against_base(std::string_view input, std::optional<std::string_view> base_url_str) {
    upa::url url;
    if (base_url_str)
        url.parse(input, *base_url_str);
    else
        url.parse(input);
    return url;
}

// Get code point from a string

template <class StrT, upa::enable_if_str_arg_t<StrT> = 0>
inline char32_t get_code_point(StrT&& input) {
    const auto inp = upa::make_str_arg(std::forward<StrT>(input));
    const auto* ptr = inp.begin();
    return upa::url_utf::read_utf_char(ptr, inp.end()).value;
}

template <class StrT, upa::enable_if_str_arg_t<StrT> = 0>
inline char32_t get_code_point(StrT&& input, std::size_t& ind) {
    const auto inp = upa::make_str_arg(std::forward<StrT>(input));
    const auto* ptr = inp.begin() + ind;
    const char32_t cp = upa::url_utf::read_utf_char(ptr, inp.end()).value;
    ind = ptr - inp.begin();
    return cp;
}

///////////////////////////////////////////////////////////////////////
// Requirements for regex_engine

template<class T, class = void>
struct has_regex_engine_members : std::false_type {};

template<class T>
struct has_regex_engine_members<T, std::void_t<
        typename T::result,
        // T::result members
        decltype(std::declval<typename T::result>().size()),
        decltype(std::declval<typename T::result>().get(std::declval<std::size_t>(),
            std::declval<std::string_view>())),
        // T members
        decltype(std::declval<T>().init(std::declval<std::string_view>(), std::declval<bool>())),
        decltype(std::declval<T>().exec(std::declval<std::string_view>(),
            std::declval<typename T::result&>())),
        decltype(std::declval<T>().test(std::declval<std::string_view>()))
    >> : std::conjunction<
        // Check the return value types of T::result members
        std::is_same<decltype(std::declval<typename T::result>().size()), std::size_t>,
        std::is_same<decltype(std::declval<typename T::result>().get(std::declval<std::size_t>(),
            std::declval<std::string_view>())), std::optional<std::string>>,
        // Check the return value types of T members
        std::is_same<decltype(std::declval<T>().init(std::declval<std::string_view>(),
            std::declval<bool>())), bool>,
        std::is_same<decltype(std::declval<T>().exec(std::declval<std::string_view>(),
            std::declval<typename T::result&>())), bool>,
        std::is_same<decltype(std::declval<T>().test(std::declval<std::string_view>())), bool>
    > {};

template<class T>
constexpr bool is_regex_engine_v =
    std::is_default_constructible_v<T>
    && std::is_copy_constructible_v<T>
    && std::is_move_constructible_v<T>
    && std::is_copy_assignable_v<T>
    && std::is_move_assignable_v<T>
    && has_regex_engine_members<T>::value;

///////////////////////////////////////////////////////////////////////
// 1. The URLPattern class
// 1.1. Introduction
// 1.2. The URLPattern class
// https://urlpattern.spec.whatwg.org/#urlpattern-class
// https://urlpattern.spec.whatwg.org/#dictdef-urlpatterninit

struct urlpattern_init {
    std::optional <std::string> protocol;
    std::optional <std::string> username;
    std::optional <std::string> password;
    std::optional <std::string> hostname;
    std::optional <std::string> port;
    std::optional <std::string> pathname;
    std::optional <std::string> search;
    std::optional <std::string> hash;
    std::optional <std::string> base_url;
};

// 1.6. Constructor string parsing
// https://urlpattern.spec.whatwg.org/#constructor-string-parsing
// https://urlpattern.spec.whatwg.org/#parse-a-constructor-string

template <class regex_engine>
inline urlpattern_init parse_constructor_string(std::string_view input);

// 2. Pattern strings
// https://urlpattern.spec.whatwg.org/#pattern-strings

// https://urlpattern.spec.whatwg.org/#pattern-string
// A pattern string is a string that is written to match a set of target strings. A well formed
// pattern string conforms to a particular pattern syntax. This pattern syntax is directly based
// on the syntax used by the popular path-to-regexp JavaScript library.
using pattern_string = std::string;
using pattern_string_view = std::string_view;

// 2.1. Parsing pattern strings
// https://urlpattern.spec.whatwg.org/#parsing-pattern-strings

// 2.1.1. Tokens
// https://urlpattern.spec.whatwg.org/#tokens

// https://urlpattern.spec.whatwg.org/#token
struct token {
    enum class type {
        OPEN,
        CLOSE,
        REGEXP,
        NAME,
        CHAR,
        ESCAPED_CHAR,
        OTHER_MODIFIER,
        ASTERISK,
        END,
        INVALID_CHAR
    };

    type type_;
    std::size_t index_;
    std::string_view value_;

};

// https://urlpattern.spec.whatwg.org/#token-list
using token_list = std::vector<token>;

// 2.1.2. Tokenizing
// https://urlpattern.spec.whatwg.org/#tokenizing

// https://urlpattern.spec.whatwg.org/#tokenize-policy
enum class tokenize_policy {
    STRICT,
    LENIENT
};

// https://urlpattern.spec.whatwg.org/#tokenize
inline token_list tokenize(std::string_view input, tokenize_policy policy);

// 2.1.3. Parts
// https://urlpattern.spec.whatwg.org/#parts

// https://urlpattern.spec.whatwg.org/#part
struct part {
    enum class type {
        /// The part represents a simple fixed text string
        FIXED_TEXT,
        /// The part represents a matching group with a custom regular expression
        REGEXP,
        /// The part represents a matching group that matches code points up to the next
        /// separator code point. This is typically used for a named group like ":foo" that
        /// does not have a custom regular expression.
        SEGMENT_WILDCARD,
        /// The part represents a matching group that greedily matches all code points.
        /// This is typically used for the "*" wildcard matching group.
        FULL_WILDCARD
    };

    enum class modifier {
        /// The part does not have a modifier.
        NONE,
        /// The part has an optional modifier indicated by the U+003F (?) code point.
        OPTIONAL,
        /// The part has a "zero or more" modifier indicated by the U+002A (*) code point.
        ZERO_OR_MORE,
        /// The part has a "one or more" modifier indicated by the U+002B (+) code point.
        ONE_OR_MORE
    };

    part(type t, std::string&& value, modifier m)
        : type_{ t }
        , value_{ std::move(value) }
        , modifier_{ m }
    {}

    type type_;
    std::string value_;
    modifier modifier_;
    std::string name_{};
    std::string prefix_{};
    std::string suffix_{};
};

// https://urlpattern.spec.whatwg.org/#part-list
using part_list = std::vector<part>;


// 2.1.4.Options
// https://urlpattern.spec.whatwg.org/#options-header

// https://urlpattern.spec.whatwg.org/#options
struct options {
    // ASCII code point or the empty string
    std::string_view delimiter_code_point{}; // TODO: maybe char32_t?
    std::string_view prefix_code_point{}; // TODO: maybe char32_t?
    bool ignore_case = false;
};

// 2.1.5. Parsing
// https://urlpattern.spec.whatwg.org/#parsing

// An encoding callback is an abstract algorithm that takes a given string input.
// The input will be a simple text piece of a pattern string. An implementing
// algorithm will validate and encode the input. It must return the encoded string
// or throw an exception
// https://urlpattern.spec.whatwg.org/#encoding-callback
using encoding_callback = std::string (*)(std::string_view input);

// https://urlpattern.spec.whatwg.org/#parse-a-pattern-string
inline part_list parse_pattern_string(pattern_string_view input, const options& opt, encoding_callback encoding_cb);

// https://urlpattern.spec.whatwg.org/#full-wildcard-regexp-value
constexpr std::string_view full_wildcard_regexp_value = ".*"sv;

// https://urlpattern.spec.whatwg.org/#generate-a-segment-wildcard-regexp
inline std::string generate_segment_wildcard_regexp(const options& opt);

// 2.2. Converting part lists to regular expressions
// https://urlpattern.spec.whatwg.org/#converting-part-lists-to-regular-expressions

using string_list = std::vector<std::string>;

// https://urlpattern.spec.whatwg.org/#generate-a-regular-expression-and-name-list
inline std::pair<std::string, string_list> generate_regular_expression_and_name_list(const part_list& pt_list, const options& opt);

inline void append_escape_regexp_string(std::string& result, std::string_view input);

// 2.3. Converting part lists to pattern strings
// https://urlpattern.spec.whatwg.org/#converting-part-lists-to-pattern-strings

inline pattern_string generate_pattern_string(const part_list& pt_list, const options& opt);

inline std::string escape_pattern_string(std::string_view input);
inline void append_escape_pattern_string(std::string& result, std::string_view input);
inline void append_convert_modifier_to_string(std::string& result, part::modifier modifier);

// 3. Canonicalization
// https://urlpattern.spec.whatwg.org/#canon

// 3.1. Encoding callbacks
// https://urlpattern.spec.whatwg.org/#canon-encoding-callbacks

inline std::string canonicalize_protocol(std::string_view value);
inline std::string canonicalize_username(std::string_view value);
inline std::string canonicalize_password(std::string_view value);
inline std::string canonicalize_hostname(std::string_view value);
inline std::string canonicalize_ipv6_hostname(std::string_view value);
inline std::string canonicalize_port(std::string_view port_value, std::optional<std::string_view> protocol_value);
inline std::string canonicalize_port(std::string_view port_value) {
    return canonicalize_port(port_value, std::nullopt);
}
inline std::string canonicalize_pathname(std::string_view value);
inline std::string canonicalize_opaque_pathname(std::string_view value);
inline std::string canonicalize_search(std::string_view value);
inline std::string canonicalize_hash(std::string_view value);

// 3.2. URLPatternInit processing
// https://urlpattern.spec.whatwg.org/#canon-processing-for-init

enum class urlpattern_init_type { PATTERN, URL };

inline urlpattern_init process_urlpattern_init(const urlpattern_init& init, urlpattern_init_type type, bool set_empty);


///////////////////////////////////////////////////////////////////////

// 1.3. The URL pattern struct
// https://urlpattern.spec.whatwg.org/#component

template <class regex_engine>
struct component {
    component() = default;
    component(component&&) noexcept = default;
    // compile a component
    component(std::optional<std::string_view> input, encoding_callback encoding_cb, const options& opt);

    component& operator=(component&&) noexcept = default;

    // well formed pattern string
    pattern_string pattern_string_;
    regex_engine regular_expression_;
    string_list group_name_list_;
    bool has_regexp_groups_ = false;
};

// 1.5. Internals
// https://urlpattern.spec.whatwg.org/#urlpattern-internals

// ....

// https://urlpattern.spec.whatwg.org/#default-options
// The default options is an options struct with delimiter code point set to the empty string and
// prefix code point set to the empty string.
inline constexpr options default_options{};

// https://urlpattern.spec.whatwg.org/#hostname-options
// TODO: if C++20 use designated initializers
inline constexpr options hostname_options { "."sv };

template <class regex_engine>
inline bool protocol_component_matches_special_scheme(const component<regex_engine>& protocol_component);
constexpr bool hostname_pattern_is_ipv6_address(pattern_string_view input) noexcept;

///////////////////////////////////////////////////////////////////////
// 1.2. The URLPattern class
// https://urlpattern.spec.whatwg.org/#urlpattern-class

using urlpattern_input = std::variant<std::string_view, const urlpattern_init*, const upa::url*>;

struct urlpattern_options {
    bool ignore_case = false;
};

struct urlpattern_component_result {
    std::string input;
    std::unordered_map<std::string_view, std::optional<std::string>> groups;
};

struct urlpattern_result {
    // TODO: Optimize as max vector size here is 2 
    std::vector<urlpattern_input> inputs;

    urlpattern_component_result protocol;
    urlpattern_component_result username;
    urlpattern_component_result password;
    urlpattern_component_result hostname;
    urlpattern_component_result port;
    urlpattern_component_result pathname;
    urlpattern_component_result search;
    urlpattern_component_result hash;
};

template <class regex_engine,
    typename = std::enable_if_t<is_regex_engine_v<regex_engine>>>
class urlpattern {
public:
    // constructor with urlpattern_init as URLPatternInput
    urlpattern(const urlpattern_init& init = {}, urlpattern_options opt = {});

    // constructor with std::string_view as URLPatternInput
    urlpattern(std::string_view input, std::optional<std::string_view> base_url, urlpattern_options opt = {});

    urlpattern(std::string_view input, urlpattern_options opt = {})
        : urlpattern(input, std::nullopt, opt) {}

    bool test(const urlpattern_init& input) const;
    bool test(std::string_view input, std::optional<std::string_view> base_url_str = std::nullopt) const;
    bool test(const upa::url& url) const;

    std::optional<urlpattern_result> exec(const urlpattern_init& input) const;
    std::optional<urlpattern_result> exec(std::string_view input,
        std::optional<std::string_view> base_url_str = std::nullopt) const;
    std::optional<urlpattern_result> exec(const upa::url& url) const;

    // returns component's pattern_string_
    pattern_string_view get_protocol() const noexcept;
    pattern_string_view get_username() const noexcept;
    pattern_string_view get_password() const noexcept;
    pattern_string_view get_hostname() const noexcept;
    pattern_string_view get_port() const noexcept;
    pattern_string_view get_pathname() const noexcept;
    pattern_string_view get_search() const noexcept;
    pattern_string_view get_hash() const noexcept;
    // Returns whether urlpattern contains one or more groups which uses regular expression matching
    bool has_regexp_groups() const noexcept;

private:
    using regex_exec_result = typename regex_engine::result;

    bool match_for_test(
        std::string_view protocol, std::string_view username, std::string_view password,
        std::string_view hostname, std::string_view port, std::string_view pathname,
        std::string_view search, std::string_view hash) const;

    std::optional<urlpattern_result> match(std::vector<urlpattern_input>&& inputs,
        std::string_view protocol, std::string_view username, std::string_view password,
        std::string_view hostname, std::string_view port, std::string_view pathname,
        std::string_view search, std::string_view hash) const;

    static urlpattern_init make_urlpattern_init(std::string_view input, std::optional<std::string_view> base_url);
    static urlpattern_component_result create_component_match_result(const component<regex_engine>& comp,
        std::string_view input, const regex_exec_result& exec_result);

    // 1.3. The URL pattern struct
    // https://urlpattern.spec.whatwg.org/#url-pattern
    component<regex_engine> protocol_component_;
    component<regex_engine> username_component_;
    component<regex_engine> password_component_;
    component<regex_engine> hostname_component_;
    component<regex_engine> port_component_;
    component<regex_engine> pathname_component_;
    component<regex_engine> search_component_;
    component<regex_engine> hash_component_;
};

///////////////////////////////////////////////////////////////////////
/// @brief urlpattern exception class

class urlpattern_error : public std::runtime_error {
public:
    /// constructs a new urlpattern_error object with the given error message
    ///
    /// @param[in] what_arg error message
    explicit urlpattern_error(const char* what_arg)
        : std::runtime_error(what_arg)
    {}
};

///////////////////////////////////////////////////////////////////////
// 1.2. The URLPattern class
// https://urlpattern.spec.whatwg.org/#urlpattern-class

// initialize (as constructors)
// https://urlpattern.spec.whatwg.org/#urlpattern-initialize

// 1.4. High-level operations: To create a URL pattern ...
// https://urlpattern.spec.whatwg.org/#url-pattern-create

template <class regex_engine, typename E>
inline urlpattern_init urlpattern<regex_engine, E>::make_urlpattern_init(std::string_view input,
    std::optional<std::string_view> base_url)
{
    urlpattern_init init{ parse_constructor_string<regex_engine>(input) };
    if (!base_url && !init.protocol)
        throw urlpattern_error("No base URL");
    init.base_url = base_url;
    return init;
}

template <class regex_engine, typename E>
inline urlpattern<regex_engine, E>::urlpattern(std::string_view input,
    std::optional<std::string_view> base_url, urlpattern_options opt)
    : urlpattern{ make_urlpattern_init(input, base_url), opt }
{}

template <class regex_engine, typename E>
inline urlpattern<regex_engine, E>::urlpattern(const urlpattern_init& init, urlpattern_options opt) {
    // Let processedInit be the result of process a URLPatternInit given init, "pattern",
    // null, null, null, null, null, null, null, and null.
    auto processed_init = process_urlpattern_init(init, urlpattern_init_type::PATTERN, false/*all nulls*/);

    // For each componentName of {"protocol", "username", "password", "hostname", "port", "pathname",
    // "search", "hash"}:
    // - If processedInit[componentName] does not exist, then set processedInit[componentName] to "*"
    if (!processed_init.protocol) processed_init.protocol = "*"sv;
    if (!processed_init.username) processed_init.username = "*"sv;
    if (!processed_init.password) processed_init.password = "*"sv;
    if (!processed_init.hostname) processed_init.hostname = "*"sv;
    if (!processed_init.port) processed_init.port = "*"sv;
    if (!processed_init.pathname) processed_init.pathname = "*"sv;
    if (!processed_init.search) processed_init.search = "*"sv;
    if (!processed_init.hash) processed_init.hash = "*"sv;

    // If processedInit["protocol"] is a special scheme and processedInit["port"] is a string
    // which represents its corresponding default port in radix-10 using ASCII digits then set
    // processedInit["port"] to the empty string
    if (is_special_scheme_default_port(*processed_init.protocol, *processed_init.port))
        processed_init.port = ""sv;

    // component constructor performs `compile a component`
    protocol_component_ = component<regex_engine>(processed_init.protocol, canonicalize_protocol, default_options);
    username_component_ = component<regex_engine>(processed_init.username, canonicalize_username, default_options);
    password_component_ = component<regex_engine>(processed_init.password, canonicalize_password, default_options);

    if (hostname_pattern_is_ipv6_address(*processed_init.hostname))
        hostname_component_ = component<regex_engine>(processed_init.hostname, canonicalize_ipv6_hostname, hostname_options);
    else
        hostname_component_ = component<regex_engine>(processed_init.hostname, canonicalize_hostname, hostname_options);

    port_component_ = component<regex_engine>(processed_init.port, canonicalize_port, default_options);

    // Let compileOptions be a copy of the default options with
    // the ignore case property set to options["ignoreCase"].
    const options compile_opt{ ""sv, ""sv, opt.ignore_case };
    if (protocol_component_matches_special_scheme(protocol_component_)) {
        // pathname options
        // https://urlpattern.spec.whatwg.org/#pathname-options
        const options path_compile_opt{ "/"sv, "/"sv, opt.ignore_case };
        pathname_component_ = component<regex_engine>(processed_init.pathname, canonicalize_pathname, path_compile_opt);
    } else {
        pathname_component_ = component<regex_engine>(processed_init.pathname, canonicalize_opaque_pathname, compile_opt);
    }
    search_component_ = component<regex_engine>(processed_init.search, canonicalize_search, compile_opt);
    hash_component_ = component<regex_engine>(processed_init.hash, canonicalize_hash, compile_opt);
}

// https://urlpattern.spec.whatwg.org/#dom-urlpattern-protocol
template <class regex_engine, typename E>
inline pattern_string_view urlpattern<regex_engine, E>::get_protocol() const noexcept {
    return protocol_component_.pattern_string_;
}
template <class regex_engine, typename E>
inline pattern_string_view urlpattern<regex_engine, E>::get_username() const noexcept {
    return username_component_.pattern_string_;
}
template <class regex_engine, typename E>
inline pattern_string_view urlpattern<regex_engine, E>::get_password() const noexcept {
    return password_component_.pattern_string_;
}
template <class regex_engine, typename E>
inline pattern_string_view urlpattern<regex_engine, E>::get_hostname() const noexcept {
    return hostname_component_.pattern_string_;
}
template <class regex_engine, typename E>
inline pattern_string_view urlpattern<regex_engine, E>::get_port() const noexcept {
    return port_component_.pattern_string_;
}
template <class regex_engine, typename E>
inline pattern_string_view urlpattern<regex_engine, E>::get_pathname() const noexcept {
    return pathname_component_.pattern_string_;
}
template <class regex_engine, typename E>
inline pattern_string_view urlpattern<regex_engine, E>::get_search() const noexcept {
    return search_component_.pattern_string_;
}
template <class regex_engine, typename E>
inline pattern_string_view urlpattern<regex_engine, E>::get_hash() const noexcept {
    return hash_component_.pattern_string_;
}

// https://urlpattern.spec.whatwg.org/#dom-urlpattern-test
// https://urlpattern.spec.whatwg.org/#url-pattern-match

template <class regex_engine, typename E>
inline bool urlpattern<regex_engine, E>::test(const urlpattern_init& input) const {
    urlpattern_init apply_result;
    try {
        apply_result = process_urlpattern_init(input, urlpattern_init_type::URL, true);
    }
    catch (std::exception&) {
        return false;
    }
    return match_for_test(
        *apply_result.protocol, *apply_result.username, *apply_result.password,
        *apply_result.hostname, *apply_result.port, *apply_result.pathname,
        *apply_result.search, *apply_result.hash);
}

template <class regex_engine, typename E>
inline bool urlpattern<regex_engine, E>::test(std::string_view input, std::optional<std::string_view> base_url_str) const {
    return test(parse_url_against_base(input, base_url_str));
}

template <class regex_engine, typename E>
inline bool urlpattern<regex_engine, E>::test(const upa::url& url) const {
    if (!url.is_valid())
        return false;

    return match_for_test(
        url.get_part_view(upa::url::SCHEME),
        url.get_part_view(upa::url::USERNAME),
        url.get_part_view(upa::url::PASSWORD),
        url.get_part_view(upa::url::HOST),
        url.get_part_view(upa::url::PORT),
        url.get_part_view(upa::url::PATH),
        url.get_part_view(upa::url::QUERY),
        url.get_part_view(upa::url::FRAGMENT));
}

template <class regex_engine, typename E>
inline bool urlpattern<regex_engine, E>::match_for_test(
    std::string_view protocol, std::string_view username, std::string_view password,
    std::string_view hostname, std::string_view port, std::string_view pathname,
    std::string_view search, std::string_view hash) const
{
    return
        protocol_component_.regular_expression_.test(protocol) &&
        username_component_.regular_expression_.test(username) &&
        password_component_.regular_expression_.test(password) &&
        hostname_component_.regular_expression_.test(hostname) &&
        port_component_.regular_expression_.test(port) &&
        pathname_component_.regular_expression_.test(pathname) &&
        search_component_.regular_expression_.test(search) &&
        hash_component_.regular_expression_.test(hash);
}

// https://urlpattern.spec.whatwg.org/#dom-urlpattern-exec
// https://urlpattern.spec.whatwg.org/#url-pattern-match

template <class regex_engine, typename E>
inline std::optional<urlpattern_result> urlpattern<regex_engine, E>::exec(const urlpattern_init& input) const {
    urlpattern_init apply_result;
    try {
        apply_result = process_urlpattern_init(input, urlpattern_init_type::URL, true);
    }
    catch (std::exception&) {
        return std::nullopt;
    }

    // Append input to inputs
    std::vector<urlpattern_input> inputs{ &input };

    return match(std::move(inputs),
        *apply_result.protocol, *apply_result.username, *apply_result.password,
        *apply_result.hostname, *apply_result.port, *apply_result.pathname,
        *apply_result.search, *apply_result.hash);
}

template <class regex_engine, typename E>
inline std::optional<urlpattern_result> urlpattern<regex_engine, E>::exec(std::string_view input,
    std::optional<std::string_view> base_url_str) const
{
    // Parse input
    upa::url url = parse_url_against_base(input, base_url_str);
    if (!url.is_valid())
        return std::nullopt;

    // Append input to inputs
    std::vector<urlpattern_input> inputs{ input };
    if (base_url_str) {
        // Append baseURLString to inputs
        inputs.push_back(*base_url_str);
    }

    return match(std::move(inputs),
        url.get_part_view(upa::url::SCHEME),
        url.get_part_view(upa::url::USERNAME),
        url.get_part_view(upa::url::PASSWORD),
        url.get_part_view(upa::url::HOST),
        url.get_part_view(upa::url::PORT),
        url.get_part_view(upa::url::PATH),
        url.get_part_view(upa::url::QUERY),
        url.get_part_view(upa::url::FRAGMENT));
}

template <class regex_engine, typename E>
inline std::optional<urlpattern_result> urlpattern<regex_engine, E>::exec(const upa::url& url) const {
    if (!url.is_valid())
        return std::nullopt;

    // Append input to inputs
    std::vector<urlpattern_input> inputs{ &url };

    return match(std::move(inputs),
        url.get_part_view(upa::url::SCHEME),
        url.get_part_view(upa::url::USERNAME),
        url.get_part_view(upa::url::PASSWORD),
        url.get_part_view(upa::url::HOST),
        url.get_part_view(upa::url::PORT),
        url.get_part_view(upa::url::PATH),
        url.get_part_view(upa::url::QUERY),
        url.get_part_view(upa::url::FRAGMENT));
}

// create a component match result
// https://urlpattern.spec.whatwg.org/#create-a-component-match-result

template <class regex_engine, typename E>
inline urlpattern_component_result urlpattern<regex_engine, E>::create_component_match_result(
    const component<regex_engine>& comp, std::string_view input,
    const regex_exec_result& exec_result)
{
    urlpattern_component_result result;
    result.input = input;

    const auto count = exec_result.size();
    for (std::size_t index = 1; index < count; ++index) {
        std::string_view name = comp.group_name_list_[index - 1];
        result.groups.emplace(name, exec_result.get(index, input));
    }
    return result;
}

template <class regex_engine, typename E>
inline std::optional<urlpattern_result> urlpattern<regex_engine, E>::match(std::vector<urlpattern_input>&& inputs,
    std::string_view protocol, std::string_view username, std::string_view password,
    std::string_view hostname, std::string_view port, std::string_view pathname,
    std::string_view search, std::string_view hash) const
{
    // Let protocolExecResult be RegExpBuiltinExec(urlpattern's protocol component's regular expression, protocol).
    regex_exec_result protocol_exec_result;
    if (!protocol_component_.regular_expression_.exec(protocol, protocol_exec_result))
        return std::nullopt;

    regex_exec_result username_exec_result;
    if (!username_component_.regular_expression_.exec(username, username_exec_result))
        return std::nullopt;

    regex_exec_result password_exec_result;
    if (!password_component_.regular_expression_.exec(password, password_exec_result))
        return std::nullopt;

    regex_exec_result hostname_exec_result;
    if (!hostname_component_.regular_expression_.exec(hostname, hostname_exec_result))
        return std::nullopt;

    regex_exec_result port_exec_result;
    if (!port_component_.regular_expression_.exec(port, port_exec_result))
        return std::nullopt;

    regex_exec_result pathname_exec_result;
    if (!pathname_component_.regular_expression_.exec(pathname, pathname_exec_result))
        return std::nullopt;

    regex_exec_result search_exec_result;
    if (!search_component_.regular_expression_.exec(search, search_exec_result))
        return std::nullopt;

    regex_exec_result hash_exec_result;
    if (!hash_component_.regular_expression_.exec(hash, hash_exec_result))
        return std::nullopt;

    // Let result be a new URLPatternResult.
    urlpattern_result result;
    result.inputs = std::move(inputs);
    result.protocol = create_component_match_result(protocol_component_, protocol, protocol_exec_result);
    result.username = create_component_match_result(username_component_, username, username_exec_result);
    result.password = create_component_match_result(password_component_, password, password_exec_result);
    result.hostname = create_component_match_result(hostname_component_, hostname, hostname_exec_result);
    result.port = create_component_match_result(port_component_, port, port_exec_result);
    result.pathname = create_component_match_result(pathname_component_, pathname, pathname_exec_result);
    result.search = create_component_match_result(search_component_, search, search_exec_result);
    result.hash = create_component_match_result(hash_component_, hash, hash_exec_result);

    return result;
}

// https://urlpattern.spec.whatwg.org/#url-pattern-has-regexp-groups

template <class regex_engine, typename E>
bool urlpattern<regex_engine, E>::has_regexp_groups() const noexcept {
    return
        protocol_component_.has_regexp_groups_ ||
        username_component_.has_regexp_groups_ ||
        password_component_.has_regexp_groups_ ||
        hostname_component_.has_regexp_groups_ ||
        port_component_.has_regexp_groups_ ||
        pathname_component_.has_regexp_groups_ ||
        search_component_.has_regexp_groups_ ||
        hash_component_.has_regexp_groups_;
}

// 1.5. Internals
// https://urlpattern.spec.whatwg.org/#urlpattern-internals

// compile a component
// https://urlpattern.spec.whatwg.org/#compile-a-component

template <class regex_engine>
inline component<regex_engine>::component(std::optional<std::string_view> input, encoding_callback encoding_cb, const options& opt) {
    // Let part list be the result of running parse a pattern string given
    // input, options, and encoding callback
    const auto pt_list = parse_pattern_string(*input, opt, encoding_cb);
    auto [regular_expression_string, name_list] =
        generate_regular_expression_and_name_list(pt_list, opt);

    // Note
    // The specification uses regular expressions to perform all matching, but this is not mandated.
    // Implementations are free to perform matching directly against the part list when possible;
    // e.g. when there are no custom regexp matching groups. If there are custom regular
    // expressions, however, its important that they be immediately evaluated in the compile
    // a component algorithm so an error can be thrown if they are invalid. 
    if (!regular_expression_.init(regular_expression_string, opt.ignore_case))
        throw urlpattern_error("regular expression is not valid");

    pattern_string_ = generate_pattern_string(pt_list, opt);
    group_name_list_ = std::move(name_list);
    has_regexp_groups_ = std::any_of(pt_list.begin(), pt_list.end(),
        [](const auto& pt) -> bool {
            return pt.type_ == part::type::REGEXP;
        });
}

// https://urlpattern.spec.whatwg.org/#protocol-component-matches-a-special-scheme

template <class regex_engine>
inline bool protocol_component_matches_special_scheme(const component<regex_engine>& protocol_component) {
#if 1
    return [](const auto& re, auto... scheme) {
        return (... || re.test(scheme));
    }(protocol_component.regular_expression_,
        "ftp"sv, "file"sv, "http"sv, "https"sv, "ws"sv, "wss"sv);
#else
    static std::string_view special_scheme_list[] = {
        "ftp"sv, "file"sv, "http"sv, "https"sv, "ws"sv, "wss"sv
    };
    for (const auto& scheme : special_scheme_list) {
        if (std::regex_match(scheme.begin(), scheme.end(), protocol_component.regular_expression_))
            return true;
    }
    return false;
#endif
}

// https://urlpattern.spec.whatwg.org/#hostname-pattern-is-an-ipv6-address

constexpr bool hostname_pattern_is_ipv6_address(pattern_string_view input) noexcept {
    // If input's code point length is less than 2, then return false.
    // TODO: code point (not necessary)
    if (input.length() < 2)
        return false;
    return input[0] == '[' ||
        (input[0] == '{' && input[1] == '[') ||
        (input[0] == '\\' && input[1] == '[');
}

// 1.6. Constructor string parsing
// https://urlpattern.spec.whatwg.org/#constructor-string-parsing

// https://urlpattern.spec.whatwg.org/#constructor-string-parser
struct constructor_string_parser {
    // https://urlpattern.spec.whatwg.org/#constructor-string-parser-state
    enum class state {
        INIT,
        PROTOCOL,
        AUTHORITY,
        USERNAME,
        PASSWORD,
        HOSTNAME,
        PORT,
        PATHNAME,
        SEARCH,
        HASH,
        DONE
    };

    constructor_string_parser(std::string_view input);

    void change_state(state new_state, std::size_t skip);
    void rewind();
    void rewind_and_set_state(state state);
    const token& get_safe_token(std::size_t index) const;
    bool is_non_special_pattern_char(std::size_t index, std::string_view value) const;
    bool is_protocol_suffix() const;
    bool next_is_authority_slashes() const;
    bool is_identity_terminator() const;
    bool is_password_prefix() const;
    bool is_port_prefix() const;
    bool is_pathname_start() const;
    bool is_search_prefix() const;
    bool is_hash_prefix() const;
    bool is_group_open() const;
    bool is_group_close() const;
    bool is_ipv6_open() const;
    bool is_ipv6_close() const;
    std::string_view make_component_string() const;
    template <class regex_engine>
    void compute_protocol_matches_special_scheme_flag();

    std::string_view input_;
    token_list token_list_;
    urlpattern_init result_;
    std::size_t component_start_ = 0;
    std::size_t token_index_ = 0;
    std::size_t token_increment_ = 1;
    std::size_t group_depth_ = 0;
    std::size_t hostname_ipv6_bracket_depth_ = 0;
    bool protocol_matches_special_scheme_flag_ = false;
    state state_ = state::INIT;
};

// https://urlpattern.spec.whatwg.org/#parse-a-constructor-string
// 1. Let parser be a new constructor string parser whose input is input and token
// list is the result of running tokenize given input and "lenient".

inline constructor_string_parser::constructor_string_parser(std::string_view input)
    : input_{ input }
    , token_list_{ tokenize(input, tokenize_policy::LENIENT) }
{}

template <class regex_engine>
inline urlpattern_init parse_constructor_string(std::string_view input) {
    using state = constructor_string_parser::state;

    constructor_string_parser parser{ input };

    // 2. While parser's token index is less than parser's token list size:
    while (parser.token_index_ < parser.token_list_.size()) {
        parser.token_increment_ = 1;
        // Note
        // On every iteration of the parse loop the parser's token index will be incremented by its
        // token increment value. Typically this means incrementing by 1, but at certain times it is
        // set to zero. The token increment is then always reset back to 1 at the top of the loop. 

        if (parser.token_list_[parser.token_index_].type_ == token::type::END) {
            if (parser.state_ == state::INIT) {
                // Note
                // If we reached the end of the string in the "init" state, then we failed to find a
                // protocol terminator and this has to be a relative URLPattern constructor string.

                parser.rewind();
                // Note
                // We next determine at which component the relative pattern begins. Relative
                // pathnames are most common, but URLs and URLPattern constructor strings can begin
                // with the search or hash components as well.

                if (parser.is_hash_prefix()) {
                    parser.change_state(state::HASH, 1);
                } else if (parser.is_search_prefix()) {
                    parser.change_state(state::SEARCH, 1);
                } else {
                    parser.change_state(state::PATHNAME, 0);
                }
                parser.token_index_ += parser.token_increment_;
                continue;
            }

            if (parser.state_ == state::AUTHORITY) {
                // Note
                // If we reached the end of the string in the "authority" state, then we failed to
                // find an "@". Therefore there is no username or password.
                parser.rewind_and_set_state(state::HOSTNAME);
                parser.token_index_ += parser.token_increment_;
                continue;
            }

            parser.change_state(state::DONE, 0);
            break;
        }

        if (parser.is_group_open()) {
            // Note
            // We ignore all code points within "{ ... }" pattern groupings. It would not make
            // sense to allow a URL component boundary to lie within a grouping; e.g.
            // "https://example.c{om/fo}o". While not supported within well formed pattern strings,
            // we handle nested groupings here to avoid parser confusion.
            //
            // It is not necessary to perform this logic for regexp or named groups since those
            // values are collapsed into individual tokens by the tokenize algorithm.
            ++parser.group_depth_;
            parser.token_index_ += parser.token_increment_;
            continue;
        }

        if (parser.group_depth_ > 0) {
            if (parser.is_group_close()) {
                --parser.group_depth_;
            } else {
                parser.token_index_ += parser.token_increment_;
                continue;
            }
        }

        switch (parser.state_) {
        case state::INIT:
            if (parser.is_protocol_suffix())
                parser.rewind_and_set_state(state::PROTOCOL);
            break;
        case state::PROTOCOL:
            if (parser.is_protocol_suffix()) {
                parser.compute_protocol_matches_special_scheme_flag<regex_engine>();
                // Note
                // We need to eagerly compile the protocol component to determine if it matches any
                // special schemes. If it does then certain special rules apply. It determines if
                // the pathname defaults to a "/" and also whether we will look for the username,
                // password, hostname, and port components. Authority slashes can also cause us to
                // look for these components as well. Otherwise we treat this as an "opaque path
                // URL" and go straight to the pathname component.
                state next_state = state::PATHNAME;
                std::size_t skip = 1;
                if (parser.next_is_authority_slashes()) {
                    next_state = state::AUTHORITY;
                    skip = 3;
                } else if (parser.protocol_matches_special_scheme_flag_) {
                    next_state = state::AUTHORITY;
                }
                parser.change_state(next_state, skip);
            }
            break;
        case state::AUTHORITY:
            if (parser.is_identity_terminator())
                parser.rewind_and_set_state(state::USERNAME);
            else if (parser.is_pathname_start() || parser.is_search_prefix() || parser.is_hash_prefix())
                parser.rewind_and_set_state(state::HOSTNAME);
            break;
        case state::USERNAME:
            if (parser.is_password_prefix())
                parser.change_state(state::PASSWORD, 1);
            else if (parser.is_identity_terminator())
                parser.change_state(state::HOSTNAME, 1);
            break;
        case state::PASSWORD:
            if (parser.is_identity_terminator())
                parser.change_state(state::HOSTNAME, 1);
            break;
        case state::HOSTNAME:
            if (parser.is_ipv6_open())
                ++parser.hostname_ipv6_bracket_depth_;
            else if (parser.is_ipv6_close())
                --parser.hostname_ipv6_bracket_depth_;
            else if (parser.is_port_prefix() && parser.hostname_ipv6_bracket_depth_ == 0)
                parser.change_state(state::PORT, 1);
            else if (parser.is_pathname_start())
                parser.change_state(state::PATHNAME, 0);
            else if (parser.is_search_prefix())
                parser.change_state(state::SEARCH, 1);
            else if (parser.is_hash_prefix())
                parser.change_state(state::HASH, 1);
            break;
        case state::PORT:
            if (parser.is_pathname_start())
                parser.change_state(state::PATHNAME, 0);
            else if (parser.is_search_prefix())
                parser.change_state(state::SEARCH, 1);
            else if (parser.is_hash_prefix())
                parser.change_state(state::HASH, 1);
            break;
        case state::PATHNAME:
            if (parser.is_search_prefix())
                parser.change_state(state::SEARCH, 1);
            else if (parser.is_hash_prefix())
                parser.change_state(state::HASH, 1);
            break;
        case state::SEARCH:
            if (parser.is_hash_prefix())
                parser.change_state(state::HASH, 1);
            break;
        case state::HASH:
            break; // Do nothing
        case state::DONE:
            assert(true); // This step is never reached
            break;
        }
        parser.token_index_ += parser.token_increment_;
    } // while

    // 3. If parser's result contains "hostname" and not "port", then set parser's
    // result["port"] to the empty string
    if (parser.result_.hostname && !parser.result_.port)
        parser.result_.port = ""sv;
    // Note
    // This is special-cased because when an author does not specify a port, they usually intend
    // the default port. If any port is acceptable, the author can specify it as a wildcard
    // explicitly. For example, "https://example.com/*" does not match URLs beginning with
    // "https://example.com:8443/", which is a different origin.

    return std::move(parser.result_);
}

// constructor_string_parser class

// https://urlpattern.spec.whatwg.org/#change-state
inline void constructor_string_parser::change_state(state new_state, std::size_t skip) {
    // If parser's state is not "init", not "authority", and not "done", then set parser's
    // result[parser's state] to the result of running make a component string given parser.
    //
    // if (state_ != state::INIT && state_ != state::AUTHORITY && state_ != state::DONE)
    switch (state_) {
    case state::PROTOCOL: result_.protocol = make_component_string(); break;
    case state::USERNAME: result_.username = make_component_string(); break;
    case state::PASSWORD: result_.password = make_component_string(); break;
    case state::HOSTNAME: result_.hostname = make_component_string(); break;
    case state::PORT: result_.port = make_component_string(); break;
    case state::PATHNAME: result_.pathname = make_component_string(); break;
    case state::SEARCH: result_.search = make_component_string(); break;
    case state::HASH: result_.hash = make_component_string(); break;
    }

    // If parser's state is not "init" and new state is not "done", then:
    if (state_ != state::INIT && new_state != state::DONE) {
        if (state_ >= state::PROTOCOL && state_ <= state::PASSWORD &&
            new_state >= state::PORT && new_state <= state::HASH &&
            !result_.hostname) {
            result_.hostname = ""sv;
        }
        if (state_ >= state::PROTOCOL && state_ <= state::PORT &&
            (new_state == state::SEARCH || new_state == state::HASH) &&
            !result_.pathname) {
            result_.pathname = protocol_matches_special_scheme_flag_ ? "/"sv : ""sv;
        }
        if (state_ >= state::PROTOCOL && state_ <= state::PATHNAME &&
            new_state == state::HASH &&
            !result_.search) {
            result_.search = ""sv;
        }
    }

    state_ = new_state;
    token_index_ += skip;
    component_start_ = token_index_;
    token_increment_ = 0;
}

// https://urlpattern.spec.whatwg.org/#rewind
inline void constructor_string_parser::rewind() {
    token_index_ = component_start_;
    token_increment_ = 0;
}

// https://urlpattern.spec.whatwg.org/#rewind-and-set-state
inline void constructor_string_parser::rewind_and_set_state(state state) {
    rewind();
    state_ = state;
}

// https://urlpattern.spec.whatwg.org/#get-a-safe-token
inline const token& constructor_string_parser::get_safe_token(std::size_t index) const {
    if (index < token_list_.size())
        return token_list_[index];

    assert(token_list_.size() >= 1);
    const auto last_index = token_list_.size() - 1;
    assert(token_list_[last_index].type_ == token::type::END);
    return token_list_[last_index];
}

// https://urlpattern.spec.whatwg.org/#is-a-non-special-pattern-char
inline bool constructor_string_parser::is_non_special_pattern_char(std::size_t index, std::string_view value) const {
    const token& tok = get_safe_token(index);
    if (tok.value_ != value)
        return false;
    return
        tok.type_ == token::type::CHAR ||
        tok.type_ == token::type::ESCAPED_CHAR ||
        tok.type_ == token::type::INVALID_CHAR;
}

// https://urlpattern.spec.whatwg.org/#is-a-protocol-suffix
inline bool constructor_string_parser::is_protocol_suffix() const {
    return is_non_special_pattern_char(token_index_, ":"sv);
}

// https://urlpattern.spec.whatwg.org/#next-is-authority-slashes
inline bool constructor_string_parser::next_is_authority_slashes() const {
    return
        is_non_special_pattern_char(token_index_ + 1, "/"sv) &&
        is_non_special_pattern_char(token_index_ + 2, "/"sv);
}

// https://urlpattern.spec.whatwg.org/#is-an-identity-terminator
inline bool constructor_string_parser::is_identity_terminator() const {
    return is_non_special_pattern_char(token_index_, "@"sv);
}

// https://urlpattern.spec.whatwg.org/#is-a-password-prefix
inline bool constructor_string_parser::is_password_prefix() const {
    return is_non_special_pattern_char(token_index_, ":"sv);
}

// https://urlpattern.spec.whatwg.org/#is-a-port-prefix
inline bool constructor_string_parser::is_port_prefix() const {
    return is_non_special_pattern_char(token_index_, ":"sv);
}

// https://urlpattern.spec.whatwg.org/#is-a-pathname-start
inline bool constructor_string_parser::is_pathname_start() const {
    return is_non_special_pattern_char(token_index_, "/"sv);
}

// https://urlpattern.spec.whatwg.org/#is-a-search-prefix
inline bool constructor_string_parser::is_search_prefix() const {
    if (is_non_special_pattern_char(token_index_, "?"sv))
        return true;
    // FIXME: maybe get_safe_token?
    if (token_list_[token_index_].value_ != "?"sv)
        return false;

    // 3. Let previous index be parser's token index - 1.
    // 4. If previous index is less than 0, then return true.
    if (token_index_ < 1)
        return true;
    const token& previous_token = get_safe_token(token_index_ - 1);
    return !(
        previous_token.type_ == token::type::NAME ||
        previous_token.type_ == token::type::REGEXP ||
        previous_token.type_ == token::type::CLOSE ||
        previous_token.type_ == token::type::ASTERISK);
}

// https://urlpattern.spec.whatwg.org/#is-a-hash-prefix
inline bool constructor_string_parser::is_hash_prefix() const {
    return is_non_special_pattern_char(token_index_, "#"sv);
}

// https://urlpattern.spec.whatwg.org/#is-a-group-open
inline bool constructor_string_parser::is_group_open() const {
    // FIXME: maybe use get_safe_token?
    return token_list_[token_index_].type_ == token::type::OPEN;
}

// https://urlpattern.spec.whatwg.org/#is-a-group-close
inline bool constructor_string_parser::is_group_close() const {
    // FIXME: maybe use get_safe_token?
    return token_list_[token_index_].type_ == token::type::CLOSE;
}

// https://urlpattern.spec.whatwg.org/#is-an-ipv6-open
inline bool constructor_string_parser::is_ipv6_open() const {
    return is_non_special_pattern_char(token_index_, "["sv);
}

// https://urlpattern.spec.whatwg.org/#is-an-ipv6-close
inline bool constructor_string_parser::is_ipv6_close() const {
    return is_non_special_pattern_char(token_index_, "]"sv);
}

// https://urlpattern.spec.whatwg.org/#make-a-component-string
inline std::string_view constructor_string_parser::make_component_string() const {
    assert(token_index_ < token_list_.size());
    const token& tok = token_list_[token_index_];
    const token& component_start_token = get_safe_token(component_start_);
    const auto component_start_input_index = component_start_token.index_;
    const auto end_index = tok.index_;
    return input_.substr(component_start_input_index, end_index - component_start_input_index);
}

// https://urlpattern.spec.whatwg.org/#compute-protocol-matches-a-special-scheme-flag
template <class regex_engine>
inline void constructor_string_parser::compute_protocol_matches_special_scheme_flag() {
    const auto protocol_string = make_component_string();
    const component<regex_engine> protocol_component{ protocol_string,  canonicalize_protocol, default_options };
    if (protocol_component_matches_special_scheme(protocol_component))
        protocol_matches_special_scheme_flag_ = true;
}

// 2.1.2. Tokenizing
// https://urlpattern.spec.whatwg.org/#tokenizing

// https://urlpattern.spec.whatwg.org/#tokenizer
struct tokenizer {
    tokenizer() = default;
    tokenizer(std::string_view input, tokenize_policy policy)
        : input_{ input }, policy_{ policy } {}

    // https://urlpattern.spec.whatwg.org/#get-the-next-code-point
    void get_the_next_code_point() {
        code_point_ = get_code_point(input_, next_index_);
    }

    // https://urlpattern.spec.whatwg.org/#seek-and-get-the-next-code-point
    void seek_and_get_the_next_code_point(std::size_t index) {
        next_index_ = index;
        get_the_next_code_point();
    }

    // https://urlpattern.spec.whatwg.org/#add-a-token
    void add_token(token::type type, std::size_t next_pos, std::size_t value_pos, std::size_t value_len) {
        token_list_.push_back({ type, index_, input_.substr(value_pos, value_len) });
        index_ = next_pos;
    }
    // https://urlpattern.spec.whatwg.org/#add-a-token-with-default-length
    void add_token_with_default_length(token::type type, std::size_t next_pos, std::size_t value_pos) {
        add_token(type, next_pos, value_pos, next_pos - value_pos);
    }
    // https://urlpattern.spec.whatwg.org/#add-a-token-with-default-position-and-length
    void add_token_with_default_position_and_length(token::type type) {
        add_token_with_default_length(type, next_index_, index_);
    }

    // https://urlpattern.spec.whatwg.org/#process-a-tokenizing-error
    void process_tokenizing_error(std::size_t next_pos, std::size_t value_pos) {
        if (policy_ == tokenize_policy::STRICT) {
            throw urlpattern_error("tokenizing error");
        }
        assert(policy_ == tokenize_policy::LENIENT);
        add_token_with_default_length(token::type::INVALID_CHAR, next_pos, value_pos);
    }

    // members
    pattern_string_view input_ {};
    tokenize_policy policy_ = tokenize_policy::STRICT;
    token_list token_list_{};
    std::size_t index_ = 0;
    std::size_t next_index_ = 0;
    // Unicode code point, initially null. But we don't need null value, because
    // tokenize function initializes it to not null before accessing it's value.
    char32_t code_point_ = 0;
};

// https://urlpattern.spec.whatwg.org/#is-a-valid-name-code-point
inline bool is_valid_name_code_point(char32_t code_point, bool first) noexcept {
    return first
        ? table::is_identifier_start(code_point)
        : table::is_identifier_part(code_point);
}

// https://urlpattern.spec.whatwg.org/#is-ascii
inline bool is_ascii(char32_t code_point) noexcept {
    return code_point >= 0 && code_point <= 0x7F;
}

// https://urlpattern.spec.whatwg.org/#tokenize
inline token_list tokenize(std::string_view input, tokenize_policy policy) {
    tokenizer tokenizer{ input, policy };

    while (tokenizer.index_ < input.length()) {
        tokenizer.seek_and_get_the_next_code_point(tokenizer.index_);

        switch(tokenizer.code_point_) {
        case '*':
            tokenizer.add_token_with_default_position_and_length(token::type::ASTERISK);
            continue;
        case '+':
        case '?':
            tokenizer.add_token_with_default_position_and_length(token::type::OTHER_MODIFIER);
            continue;
        case '\\': {
            if (tokenizer.index_ == input.length() - 1) {
                tokenizer.process_tokenizing_error(tokenizer.next_index_, tokenizer.index_);
                continue;
            }
            const auto escaped_index = tokenizer.next_index_;
            tokenizer.get_the_next_code_point();
            tokenizer.add_token_with_default_length(token::type::ESCAPED_CHAR,tokenizer.next_index_,escaped_index);
            continue;
        }
        case '{':
            tokenizer.add_token_with_default_position_and_length(token::type::OPEN);
            continue;
        case '}':
            tokenizer.add_token_with_default_position_and_length(token::type::CLOSE);
            continue;
        case ':': {
            auto name_pos = tokenizer.next_index_;
            const auto name_start = name_pos;
            while (name_pos < input.length()) {
                tokenizer.seek_and_get_the_next_code_point(name_pos);
                const bool first_code_point = name_pos == name_start;
                if (!is_valid_name_code_point(tokenizer.code_point_, first_code_point))
                    break;
                name_pos = tokenizer.next_index_;
            }
            if (name_pos <= name_start) {
                tokenizer.process_tokenizing_error(name_start, tokenizer.index_);
                continue;
            }
            tokenizer.add_token_with_default_length(token::type::NAME, name_pos, name_start);
            continue;
        }
        case '(': {
            std::size_t depth = 1;
            auto regexp_pos = tokenizer.next_index_;
            const auto regexp_start = regexp_pos;
            bool error = false;

            while (regexp_pos < input.length()) {
                tokenizer.seek_and_get_the_next_code_point(regexp_pos);
                if (!is_ascii(tokenizer.code_point_)) {
                    tokenizer.process_tokenizing_error(regexp_start, tokenizer.index_);
                    error = true;
                    break;
                }
                if (regexp_pos == regexp_start && tokenizer.code_point_ == '?') {
                    tokenizer.process_tokenizing_error(regexp_start, tokenizer.index_);
                    error = true;
                    break;
                }
                if (tokenizer.code_point_ == '\\') {
                    if (regexp_pos == input.length() - 1) {
                        tokenizer.process_tokenizing_error(regexp_start, tokenizer.index_);
                        error = true;
                        break;
                    }
                    tokenizer.get_the_next_code_point();
                    if (!is_ascii(tokenizer.code_point_)) {
                        tokenizer.process_tokenizing_error(regexp_start, tokenizer.index_);
                        error = true;
                        break;
                    }
                    regexp_pos = tokenizer.next_index_;
                    continue;
                }
                if (tokenizer.code_point_ == ')') {
                    if (--depth == 0) {
                        regexp_pos = tokenizer.next_index_;
                        break;
                    }
                } else if (tokenizer.code_point_ == '(') {
                    ++depth;
                    if (regexp_pos == input.length() - 1) {
                        tokenizer.process_tokenizing_error(regexp_start, tokenizer.index_);
                        error = true;
                        break;
                    }
                    const auto temporary_pos = tokenizer.next_index_;
                    tokenizer.get_the_next_code_point();
                    if (tokenizer.code_point_ != '?') {
                        tokenizer.process_tokenizing_error(regexp_start, tokenizer.index_);
                        error = true;
                        break;
                    }
                    tokenizer.next_index_ = temporary_pos;
                }
                regexp_pos = tokenizer.next_index_;
            }
            if (error)
                continue;
            if (depth) {
                tokenizer.process_tokenizing_error(regexp_start, tokenizer.index_);
                continue;
            }
            const auto regexp_len = regexp_pos - regexp_start - 1;
            if (regexp_len == 0) {
                tokenizer.process_tokenizing_error(regexp_start, tokenizer.index_);
                continue;
            }
            tokenizer.add_token(token::type::REGEXP, regexp_pos, regexp_start, regexp_len);
            continue;
        }
        }
        tokenizer.add_token_with_default_position_and_length(token::type::CHAR);
    } // while

    tokenizer.add_token_with_default_length(token::type::END, tokenizer.index_, tokenizer.index_);

    return std::move(tokenizer.token_list_);
}


// 2.1.5. Parsing
// https://urlpattern.spec.whatwg.org/#parsing

// https://urlpattern.spec.whatwg.org/#pattern-parser
struct pattern_parser {
    pattern_parser(encoding_callback encoding_cb, std::string_view segment_wildcard_regexp)
        : encoding_cb_(encoding_cb)
        , segment_wildcard_regexp_(segment_wildcard_regexp)
    {}

    const token* try_consume_token(token::type type);
    const token* try_consume_modifier_token();
    const token* try_consume_regexp_or_wildcard_token(const token* pname_token);
    const token& consume_required_token(token::type type);
    std::string consume_text();
    void maybe_add_part_from_pending_fixed_value();
    void add_part(std::string_view prefix, const token* pname_token, const token* pregexp_or_wildcard_token,
        std::string_view suffix, const token* pmodifier_token);
    bool is_duplicate_name(std::string_view name) const noexcept;

    // members

    token_list token_list_{};
    encoding_callback encoding_cb_;
    std::string segment_wildcard_regexp_;
    part_list part_list_{};
    std::string pending_fixed_value_{};
    std::size_t index_ = 0;
    std::size_t next_numeric_name_ = 0;
};

// https://urlpattern.spec.whatwg.org/#parse-a-pattern-string
inline part_list parse_pattern_string(pattern_string_view input, const options& opt, encoding_callback encoding_cb)
{
    pattern_parser parser{ encoding_cb, generate_segment_wildcard_regexp(opt) };
    parser.token_list_ = tokenize(input, tokenize_policy::STRICT);

    while (parser.index_ < parser.token_list_.size()) {
        // Example
        // This first section is looking for the sequence: <prefix char><name><regexp><modifier>.
        // There could be zero to all of these tokens.
        // TODO?: EXAMPLE 1
        const auto* pchar_token = parser.try_consume_token(token::type::CHAR);
        const auto* pname_token = parser.try_consume_token(token::type::NAME);
        const auto* pregexp_or_wildcard_token = parser.try_consume_regexp_or_wildcard_token(pname_token);

        if (pname_token || pregexp_or_wildcard_token) {
            // Note
            // If there is a matching group, we need to add the part immediately.
            std::string_view prefix{};
            if (pchar_token)
                prefix = pchar_token->value_;
            if (!prefix.empty() && prefix != opt.prefix_code_point) {
                parser.pending_fixed_value_.append(prefix);
                prefix = {}; // set prefix to the empty string
            }
            parser.maybe_add_part_from_pending_fixed_value();
            const auto* pmodifier_token = parser.try_consume_modifier_token();
            parser.add_part(prefix, pname_token, pregexp_or_wildcard_token, {}, pmodifier_token);
            continue;
        }

        const auto* pfixed_token = pchar_token;
        // Note
        // If there was no matching group, then we need to buffer any fixed text. We want
        // to collect as much text as possible before adding it as a "fixed-text" part.
        if (pfixed_token == nullptr)
            pfixed_token = parser.try_consume_token(token::type::ESCAPED_CHAR);
        if (pfixed_token) {
            parser.pending_fixed_value_.append(pfixed_token->value_);
            continue;
        }

        const auto* popen_token = parser.try_consume_token(token::type::OPEN);
        // Example
        // Next we look for the sequence
        // <open><char prefix><name><regexp><char suffix><close><modifier>.
        // The open and close are necessary, but the other tokens are not.
        // TODO?: EXAMPLE 2
        if (popen_token) {
            auto prefix = parser.consume_text();
            pname_token = parser.try_consume_token(token::type::NAME);
            pregexp_or_wildcard_token = parser.try_consume_regexp_or_wildcard_token(pname_token);
            auto suffix = parser.consume_text();
            parser.consume_required_token(token::type::CLOSE);
            const auto* pmodifier_token = parser.try_consume_modifier_token();
            parser.add_part(prefix, pname_token, pregexp_or_wildcard_token, suffix, pmodifier_token);
            continue;
        }

        parser.maybe_add_part_from_pending_fixed_value();
        parser.consume_required_token(token::type::END);
    } // while

    return std::move(parser.part_list_);
}

// https://urlpattern.spec.whatwg.org/#generate-a-segment-wildcard-regexp
inline std::string generate_segment_wildcard_regexp(const options& opt) {
    std::string result{"[^"};
    append_escape_regexp_string(result, opt.delimiter_code_point);
    result.append("]+?");
    return result;
}

// https://urlpattern.spec.whatwg.org/#try-to-consume-a-token
inline const token* pattern_parser::try_consume_token(token::type type) {
    // Assert: parser's index is less than parser's token list size.
    assert(index_ < token_list_.size());

    const auto& next_token = token_list_[index_];
    if (next_token.type_ != type)
        return nullptr;
    ++index_;
    return &next_token;
}

// https://urlpattern.spec.whatwg.org/#try-to-consume-a-modifier-token
inline const token* pattern_parser::try_consume_modifier_token() {
    const auto* ptoken = try_consume_token(token::type::OTHER_MODIFIER);
    if (ptoken)
        return ptoken;
    return try_consume_token(token::type::ASTERISK);
}

// https://urlpattern.spec.whatwg.org/#try-to-consume-a-regexp-or-wildcard-token
inline const token* pattern_parser::try_consume_regexp_or_wildcard_token(const token* pname_token) {
    const auto* ptoken = try_consume_token(token::type::REGEXP);
    if (pname_token == nullptr && ptoken == nullptr)
        return try_consume_token(token::type::ASTERISK);
    return ptoken;
}

// https://urlpattern.spec.whatwg.org/#consume-a-required-token
inline const token& pattern_parser::consume_required_token(token::type type) {
    const auto* ptoken = try_consume_token(type);
    if (ptoken == nullptr) {
        throw urlpattern_error("missing required token");
    }
    return *ptoken;
}

// https://urlpattern.spec.whatwg.org/#consume-text
inline std::string pattern_parser::consume_text() {
    std::string result{};
    while (true) {
        const auto* ptoken = try_consume_token(token::type::CHAR);
        if (ptoken == nullptr) {
            ptoken = try_consume_token(token::type::ESCAPED_CHAR);
            if (ptoken == nullptr) break;
        }
        result.append(ptoken->value_);
    }
    return result;
}

// https://urlpattern.spec.whatwg.org/#maybe-add-a-part-from-the-pending-fixed-value
inline void pattern_parser::maybe_add_part_from_pending_fixed_value() {
    if (pending_fixed_value_.empty())
        return;
    auto encoded_value = encoding_cb_(pending_fixed_value_);
    pending_fixed_value_.clear(); // set to the empty string
    part_list_.emplace_back(part::type::FIXED_TEXT, std::move(encoded_value), part::modifier::NONE);
}

// https://urlpattern.spec.whatwg.org/#add-a-part
inline void pattern_parser::add_part(std::string_view prefix, const token* pname_token,
    const token* pregexp_or_wildcard_token, std::string_view suffix,
    const token* pmodifier_token)
{
    part::modifier modifier = part::modifier::NONE;
    if (pmodifier_token) {
        if (pmodifier_token->value_ == "?"sv)
            modifier = part::modifier::OPTIONAL;
        else if (pmodifier_token->value_ == "*"sv)
            modifier = part::modifier::ZERO_OR_MORE;
        else if (pmodifier_token->value_ == "+"sv)
            modifier = part::modifier::ONE_OR_MORE;
    }
    if (pname_token == nullptr && pregexp_or_wildcard_token == nullptr && modifier == part::modifier::NONE) {
        // Note
        // This was a "{foo}" grouping. We add this to the pending fixed value
        // so that it will be combined with any previous or subsequent text.
        pending_fixed_value_.append(prefix);
        return;
    }
    maybe_add_part_from_pending_fixed_value();
    if (pname_token == nullptr && pregexp_or_wildcard_token == nullptr) {
        // Note
        // This was a "{foo}?" grouping. The modifier means we cannot combine
        // it with other text. Therefore we add it as a part immediately.
        assert(suffix.empty());
        if (prefix.empty())
            return;
        auto encoded_value = encoding_cb_(prefix);
        part_list_.emplace_back(part::type::FIXED_TEXT, std::move(encoded_value), modifier);
        return;
    }

    std::string_view regexp_value{};

    // Note
    // Next, we convert the regexp or wildcard token into a regular expression.
    if (pregexp_or_wildcard_token == nullptr)
        regexp_value = segment_wildcard_regexp_;
    else if (pregexp_or_wildcard_token->type_ == token::type::ASTERISK)
        regexp_value = full_wildcard_regexp_value;
    else
        regexp_value = pregexp_or_wildcard_token->value_;

    part::type type = part::type::REGEXP;

    // Note
    // Next, we convert regexp value into a part type. We make sure to go to a regular
    // expression first so that an equivalent "regexp" token will be treated the same
    // as a "name" or "asterisk" token.
    if (regexp_value == segment_wildcard_regexp_) {
        type = part::type::SEGMENT_WILDCARD;
        regexp_value = ""sv; // set to the empty string
    } else if (regexp_value == full_wildcard_regexp_value) {
        type = part::type::FULL_WILDCARD;
        regexp_value = ""sv; // set to the empty string
    }

    std::string name{};

    // Note
    // Next, we determine the part name. This can be explicitly provided by a
    // "name" token or be automatically assigned.
    if (pname_token) {
        name = pname_token->value_;
    } else if (pregexp_or_wildcard_token) {
        name = std::to_string(next_numeric_name_);
        ++next_numeric_name_;
    }
    if (is_duplicate_name(name))
        throw urlpattern_error("duplicate part name");

    // Note
    // Finally, we encode the fixed text values and create the part.
    part pt(type, std::string{ regexp_value }, modifier);
    pt.name_ = std::move(name);
    pt.prefix_ = encoding_cb_(prefix);
    pt.suffix_ = encoding_cb_(suffix);
    part_list_.push_back(std::move(pt));
}

// https://urlpattern.spec.whatwg.org/#is-a-duplicate-name
inline bool pattern_parser::is_duplicate_name(std::string_view name) const noexcept {
    return std::any_of(part_list_.begin(), part_list_.end(), [&name](const part& pt) {
        return pt.name_ == name;
    });
}

// 2.2. Converting part lists to regular expressions
// https://urlpattern.spec.whatwg.org/#converting-part-lists-to-regular-expressions

// https://urlpattern.spec.whatwg.org/#generate-a-regular-expression-and-name-list
inline std::pair<std::string, string_list> generate_regular_expression_and_name_list(
    const part_list& pt_list, const options& opt)
{
    std::string result{"^"};
    string_list name_list{};

    // TODO?: use std::format instead of append, push_back

    for (const auto& pt : pt_list) {
        if (pt.type_ == part::type::FIXED_TEXT) {
            if (pt.modifier_ == part::modifier::NONE) {
                append_escape_regexp_string(result, pt.value_);
            } else {
                // Note
                // A "fixed-text" part with a modifier uses a non capturing group.
                // It uses the following form:
                // (?:<fixed text>)<modifier>
                result.append("(?:");
                append_escape_regexp_string(result, pt.value_);
                result.push_back(')');
                append_convert_modifier_to_string(result, pt.modifier_);
            }
            continue;
        }

        assert(!pt.name_.empty());
        name_list.emplace_back(pt.name_);
        // Note
        // We collect the list of matching group names in a parallel list. This is largely done for
        // legacy reasons to match path-to-regexp. We could attempt to convert this to use regular
        // expression named captured groups, but given the complexity of this algorithm there is a
        // real risk of introducing unintended bugs. In addition, if we ever end up exposing the
        // generated regular expressions to the web we would like to maintain compability with
        // path-to-regexp which has indicated its unlikely to switch to using named capture groups.
        std::string_view regexp_value{ pt.value_};
        std::string regexp_value_buffer;
        if (pt.type_ == part::type::SEGMENT_WILDCARD) {
            regexp_value_buffer = generate_segment_wildcard_regexp(opt);
            regexp_value = regexp_value_buffer;
        } else if (pt.type_ == part::type::FULL_WILDCARD) {
            regexp_value = full_wildcard_regexp_value;
        }

        if (pt.prefix_.empty() && pt.suffix_.empty()) {
            // Note
            // If there is no prefix or suffix then generation depends on the modifier. If there
            // is no modifier or just the optional modifier, it uses the following simple form:
            // (<regexp value>)<modifier>
            //
            // If there is a repeating modifier, however, we will use the more complex form:
            // ((?:<regexp value>)<modifier>)
            if (pt.modifier_ == part::modifier::NONE || pt.modifier_ == part::modifier::OPTIONAL) {
                result.push_back('(');
                result.append(regexp_value);
                result.push_back(')');
                append_convert_modifier_to_string(result, pt.modifier_);
            } else {
                result.append("((?:");
                result.append(regexp_value);
                result.push_back(')');
                append_convert_modifier_to_string(result, pt.modifier_);
                result.push_back(')');
            }
            continue;
        }

        if (pt.modifier_ == part::modifier::NONE || pt.modifier_ == part::modifier::OPTIONAL) {
            // Note
            // This section handles non-repeating parts with a prefix or suffix. There is an inner capturing
            // group that contains the primary regexp value. The inner group is then combined with the prefix
            // or suffix in an outer non-capturing group. Finally the modifier is applied. The resulting form
            // is as follows.
            // (?:<prefix>(<regexp value>)<suffix>)<modifier>
            result.append("(?:");
            append_escape_regexp_string(result, pt.prefix_);
            result.push_back('(');
            result.append(regexp_value);
            result.push_back(')');
            append_escape_regexp_string(result, pt.suffix_);
            result.push_back(')');
            append_convert_modifier_to_string(result, pt.modifier_);
            continue;
        }

        assert(pt.modifier_ == part::modifier::ZERO_OR_MORE || pt.modifier_ == part::modifier::ONE_OR_MORE);
        assert(!pt.prefix_.empty() || !pt.suffix_.empty());
        // Note
        // Repeating parts with a prefix or suffix are dramatically more complicated. We want to exclude
        // the initial prefix and the final suffix, but include them between any repeated elements. To achieve
        // this we provide a separate initial expression that excludes the prefix. Then the expression is
        // duplicated with the prefix/suffix values included in an optional repeating element. If zero values
        // are permitted then a final optional modifier can be appended. The resulting form is as follows.
        // (?:<prefix>((?:<regexp value>)(?:<suffix><prefix>(?:<regexp value>))*)<suffix>)?
        result.append("(?:");
        append_escape_regexp_string(result, pt.prefix_);
        result.append("((?:");
        result.append(regexp_value);
        result.append(")(?:");
        append_escape_regexp_string(result, pt.suffix_);
        append_escape_regexp_string(result, pt.prefix_);
        result.append("(?:");
        result.append(regexp_value);
        result.append("))*)");
        append_escape_regexp_string(result, pt.suffix_);
        result.push_back(')');
        if (pt.modifier_ == part::modifier::ZERO_OR_MORE)
        result.push_back('?');
    } // for

    result.push_back('$');

    return { result, name_list };
}

// https://urlpattern.spec.whatwg.org/#escape-a-regexp-string
inline constexpr upa::code_point_set escape_regexp_set{ [](upa::code_point_set& self) constexpr {
    self.include({
        // . + * ? ^ $ { }
        0x2E, 0x2B, 0x2A, 0x3F, 0x5E, 0x24, 0x7B, 0x7D,
        // ( ) [ ] | / '\'
        0x28, 0x29, 0x5B, 0x5D, 0x7C, 0x2F, 0x5C });
} };

inline void append_escape_regexp_string(std::string& result, std::string_view input) {
    //TODO: assert(is_ascii_string(input));
    //TODO: optimize
    for (const auto c : input) {
        if (escape_regexp_set[c])
            result.push_back('\\');
        result.push_back(c);
    }
}


// 2.3. Converting part lists to pattern strings
// https://urlpattern.spec.whatwg.org/#converting-part-lists-to-pattern-strings

// https://urlpattern.spec.whatwg.org/#generate-a-pattern-string
inline pattern_string generate_pattern_string(const part_list& pt_list, const options& opt) {
    std::string result{};

    for (std::size_t index = 0; index < pt_list.size(); ++index) {
        const auto& pt = pt_list[index];
        // pprevious_pt, pnext_pt will be defined below
        if (pt.type_ == part::type::FIXED_TEXT) {
            if (pt.modifier_ == part::modifier::NONE) {
                append_escape_pattern_string(result, pt.value_);
                continue;
            }
            result.push_back('{');
            append_escape_pattern_string(result, pt.value_);
            result.push_back('}');
            append_convert_modifier_to_string(result, pt.modifier_);
            continue;
        }

        const auto* pprevious_pt = index > 0 ? &pt_list[index - 1] : nullptr;
        const auto* pnext_pt = index < pt_list.size() - 1 ? &pt_list[index + 1] : nullptr;

        assert(!pt.name_.empty()); // MANO
        const bool custom_name = !upa::detail::is_ascii_digit(pt.name_[0]);
        bool needs_grouping = !pt.suffix_.empty() ||
            (!pt.prefix_.empty() && pt.prefix_ != opt.prefix_code_point);

        if (!needs_grouping && custom_name &&
            pt.type_ == part::type::SEGMENT_WILDCARD && pt.modifier_ == part::modifier::NONE &&
            pnext_pt != nullptr && pnext_pt->prefix_.empty() && pnext_pt->suffix_.empty())
        {
            if (pnext_pt->type_ == part::type::FIXED_TEXT)
                needs_grouping = is_valid_name_code_point(get_code_point(pnext_pt->value_), false);
            else
                needs_grouping = upa::detail::is_ascii_digit(pnext_pt->name_[0]);
        }
        if (!needs_grouping && pt.prefix_.empty() &&
            pprevious_pt != nullptr && pprevious_pt->type_ == part::type::FIXED_TEXT &&
            // previous part's value's last code point is options's prefix code point
            pprevious_pt->value_.substr(pprevious_pt->value_.length() - 1)/*TODO: code point (not necessary)*/ == opt.prefix_code_point)
            needs_grouping = true;

        assert(/*TODO???: pt.name_ != nullptr && */ !pt.name_.empty());

        if (needs_grouping)
            result.push_back('{');

        append_escape_pattern_string(result, pt.prefix_);
        if (custom_name) {
            result.push_back(':');
            result.append(pt.name_);
        }

        switch (pt.type_) {
        case part::type::REGEXP:
            result.push_back('(');
            result.append(pt.value_);
            result.push_back(')');
            break;
        case part::type::SEGMENT_WILDCARD:
            // 14. custom_name is false
            if (!custom_name) {
                result.push_back('(');
                // TODO: append_generate_segment_wildcard_regexp
                result.append(generate_segment_wildcard_regexp(opt));
                result.push_back(')');
            }
            // 16. custom_name is true
            else if (!pt.suffix_.empty() && is_valid_name_code_point(get_code_point(pt.suffix_), false)) {
                result.push_back('\\');
            }
            break;
        case part::type::FULL_WILDCARD:
            if (!custom_name && (
                pprevious_pt == nullptr ||
                pprevious_pt->type_ == part::type::FIXED_TEXT ||
                pprevious_pt->modifier_ != part::modifier::NONE ||
                needs_grouping ||
                !pt.prefix_.empty())) {
                result.push_back('*');
            } else {
                result.push_back('(');
                result.append(full_wildcard_regexp_value);
                result.push_back(')');
            }
            break;
        }

        // 17. Append the result of running escape a pattern string given part's suffix
        // to the end of result.
        append_escape_pattern_string(result, pt.suffix_);

        if (needs_grouping)
            result.push_back('}');

        append_convert_modifier_to_string(result, pt.modifier_);
    } // for

    return result;
}

// https://urlpattern.spec.whatwg.org/#escape-a-pattern-string
inline constexpr upa::code_point_set escape_pattern_set{ [](upa::code_point_set& self) constexpr {
    self.include({
        // + * ? : { } ( ) '\'
        0x2B, 0x2A, 0x3F, 0x3A, 0x7B, 0x7D, 0x28, 0x29, 0x5C });
} };

inline std::string escape_pattern_string(std::string_view input) {
    std::string result{};
    append_escape_pattern_string(result, input);
    return result;
}

inline void append_escape_pattern_string(std::string& result, std::string_view input) {
    //TODO: assert(is_ascii_string(input));
    //TODO: optimize
    for (const auto c : input) {
        if (escape_pattern_set[c])
            result.push_back('\\');
        result.push_back(c);
    }
}

// https://urlpattern.spec.whatwg.org/#convert-a-modifier-to-a-string
inline void append_convert_modifier_to_string(std::string& result, part::modifier modifier) {
    switch (modifier) {
    case part::modifier::ZERO_OR_MORE:
        result.push_back('*');
        break;
    case part::modifier::OPTIONAL:
        result.push_back('?');
        break;
    case part::modifier::ONE_OR_MORE:
        result.push_back('+');
        break;
    }
}

// 3. Canonicalization
// https://urlpattern.spec.whatwg.org/#canon

// 3.1. Encoding callbacks
// https://urlpattern.spec.whatwg.org/#canon-encoding-callbacks

// TODO: (optimize): write a protocol parser to avoid having to use a URL parser

// https://urlpattern.spec.whatwg.org/#canonicalize-a-protocol
inline std::string canonicalize_protocol(std::string_view value) {
    if (value.empty()) return {};

    // TODO: concatenate strings
    std::string inp(value);
    inp.append("://dummy.test");

    // Note, state override is not used here because it enforces restrictions that are only appropriate for the
    // protocol setter. Instead we use the protocol to parse a dummy URL using the normal parsing entry point.
    upa::url dummy_url;
    if (!upa::success(dummy_url.parse(inp, nullptr)))
        throw urlpattern_error("invalid protocol");
    return std::string{ dummy_url.get_part_view(upa::url::SCHEME) };
}

// https://urlpattern.spec.whatwg.org/#canonicalize-a-username
inline std::string canonicalize_username(std::string_view value) {
    if (value.empty()) return {};

    std::string result;
    upa::detail::append_utf8_percent_encoded(value.data(), value.data() + value.size(), upa::userinfo_no_encode_set, result);
    return result;
}

// https://urlpattern.spec.whatwg.org/#canonicalize-a-password
inline std::string canonicalize_password(std::string_view value) {
    return canonicalize_username(value);
}

// https://urlpattern.spec.whatwg.org/#canonicalize-a-hostname
inline std::string canonicalize_hostname(std::string_view value) {
    if (value.empty()) return {};

    upa::url dummy_url{};
    {
        upa::detail::url_serializer urls(dummy_url);

        urls.set_scheme("http"); // MANO: SPEC-BUG

        const auto inp = upa::make_str_arg(value);
#if 0
        const auto parse_result = upa::detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr,
            upa::detail::url_parser::hostname_state);
#else
        // SPEC-BUG: Use https://url.spec.whatwg.org/#concept-host-parser instead
        // of "basic URL parser" (with hostname_state) to pass tests
        const auto parse_result = upa::detail::url_parser::parse_host(urls, inp.begin(), inp.end());
#endif
        if (!upa::success(parse_result))
            throw urlpattern_error("canonicalize a hostname error");
    }
    return std::string{ dummy_url.get_part_view(upa::url::HOST) };
}

// https://urlpattern.spec.whatwg.org/#canonicalize-an-ipv6-hostname
inline std::string canonicalize_ipv6_hostname(std::string_view value) {
    std::string result;

    // TODO: code point (not necessary)
    for (const auto cp : value) {
        if (!upa::detail::is_hex_char(cp) && cp != '[' && cp != ']' && cp != ':')
            throw urlpattern_error("canonicalize an IPv6 hostname error");
        result.push_back(upa::util::ascii_to_lower_char(cp));
    }
    return result;
}

// https://urlpattern.spec.whatwg.org/#canonicalize-a-port
inline std::string canonicalize_port(std::string_view port_value, std::optional<std::string_view> protocol_value) {
    if (port_value.empty()) return {};

    // SPEC-BUG: To pass tests all charcters must be digits
    if (!std::all_of(port_value.begin(), port_value.end(), upa::detail::is_ascii_digit<char>))
        throw urlpattern_error("canonicalize a port error");

    upa::url dummy_url{};
    {
        upa::detail::url_serializer urls(dummy_url);
        if (protocol_value)
            urls.set_scheme(*protocol_value);
        // Note, we set the URL record's scheme in order for the basic URL parser
        // to recognize and normalize default port values.

        const auto inp = upa::make_str_arg(port_value);
        const auto parse_result = upa::detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr,
            upa::detail::url_parser::port_state);
        if (!upa::success(parse_result))
            throw urlpattern_error("canonicalize a port error");
    }
    return std::string{ dummy_url.get_part_view(upa::url::PORT) };
}

// https://urlpattern.spec.whatwg.org/#canonicalize-a-pathname
inline std::string canonicalize_pathname(std::string_view value) {
    if (value.empty()) return {};

    // Let leading slash be true if the first code point
    // in value is U+002F (/) and otherwise false
    const bool leading_slash = value[0] == '/';
    // Let modified value be "/-" if leading slash is false
    // and otherwise the empty string.
    std::string modified_value{ leading_slash ? ""sv : "/-"sv };
    // Note
    // The URL parser will automatically prepend a leading slash to the canonicalized pathname.
    // This does not work here unfortunately. This algorithm is called for pieces of the pathname,
    // instead of the entire pathname, when used as an encoding callback. Therefore we disable the
    // prepending of the slash by inserting our own. An additional character is also inserted here
    // in order to avoid inadvertantly collapsing a leading dot due to the fake leading slash
    // being interpreted as a "/." sequence. These inserted characters are then removed from the
    // result below.
    //
    // Note, implementations are free to simply disable slash prepending in their URL parsing code
    // instead of paying the performance penalty of inserting and removing characters in this
    // algorithm.

    modified_value.append(value);

    upa::url dummy_url{};
    {
        upa::detail::url_serializer urls(dummy_url);
        const auto inp = upa::make_str_arg(modified_value);
        const auto parse_result = upa::detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr,
            upa::detail::url_parser::path_start_state);
        if (!upa::success(parse_result))
            throw urlpattern_error("canonicalize a pathname error");
    }
    auto result = dummy_url.get_part_view(upa::url::PATH);
    // If leading slash is false, then set result to the code point
    // substring from 2 to the end of the string within result.
    if (!leading_slash) result.remove_prefix(2);

    return std::string{ result };
}

// https://urlpattern.spec.whatwg.org/#canonicalize-an-opaque-pathname
inline std::string canonicalize_opaque_pathname(std::string_view value) {
    if (value.empty()) return {};

    upa::url dummy_url{};
    {
        upa::detail::url_serializer urls(dummy_url);
        // Set dummyURL's path to the empty string (so path becomes opaque,
        // see: https://url.spec.whatwg.org/#url-opaque-path)
        urls.set_has_opaque_path();

        const auto inp = upa::make_str_arg(value);
        const auto parse_result = upa::detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr,
            upa::detail::url_parser::opaque_path_state);
        if (!upa::success(parse_result))
            throw urlpattern_error("canonicalize an opaque pathname error");
    }
    return std::string{ dummy_url.get_part_view(upa::url::PATH) };
}

// https://urlpattern.spec.whatwg.org/#canonicalize-a-search
inline std::string canonicalize_search(std::string_view value) {
    if (value.empty()) return {};

    upa::url dummy_url{};
    {
        upa::detail::url_serializer urls(dummy_url);
        // Set dummyURL's query to the empty string.
        //TODO: urls.set_flag(upa::url::QUERY_FLAG);

        const auto inp = upa::make_str_arg(value);
        const auto parse_result = upa::detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr,
            upa::detail::url_parser::query_state);
        if (!upa::success(parse_result))
            throw urlpattern_error("canonicalize a search error");
    }
    return std::string{ dummy_url.get_part_view(upa::url::QUERY) };
}

// https://urlpattern.spec.whatwg.org/#canonicalize-a-hash
inline std::string canonicalize_hash(std::string_view value) {
    if (value.empty()) return {};

    upa::url dummy_url{};
    {
        upa::detail::url_serializer urls(dummy_url);
        // Set dummyURL's fragment to the empty string.
        //TODO: urls.set_flag(upa::url::FRAGMENT_FLAG);

        const auto inp = upa::make_str_arg(value);
        const auto parse_result = upa::detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr,
            upa::detail::url_parser::fragment_state);
        if (!upa::success(parse_result))
            throw urlpattern_error("canonicalize a hash error");
    }
    return std::string{ dummy_url.get_part_view(upa::url::FRAGMENT) };
}


// 3.2. URLPatternInit processing
// https://urlpattern.spec.whatwg.org/#canon-processing-for-init

inline std::string process_base_url_string(std::string_view input, urlpattern_init_type type);
inline bool is_absolute_pathname(pattern_string_view input, urlpattern_init_type type) noexcept;
inline std::string process_protocol_for_init(std::string_view value, urlpattern_init_type type);
inline std::string process_username_for_init(std::string_view value, urlpattern_init_type type);
inline std::string process_password_for_init(std::string_view value, urlpattern_init_type type);
inline std::string process_hostname_for_init(std::string_view value, urlpattern_init_type type);
inline std::string process_port_for_init(std::string_view port_value,
    std::optional<std::string_view> protocol_value, urlpattern_init_type type);
inline std::string process_pathname_for_init(std::string_view pathname_value,
    std::optional<std::string_view> protocol_value, urlpattern_init_type type);
inline std::string process_search_for_init(std::string_view value, urlpattern_init_type type);
inline std::string process_hash_for_init(std::string_view value, urlpattern_init_type type);

// https://urlpattern.spec.whatwg.org/#process-a-urlpatterninit
inline urlpattern_init process_urlpattern_init(const urlpattern_init& init, urlpattern_init_type type, bool set_empty) {
    urlpattern_init result;

    if (set_empty) {
        result.protocol = ""sv;
        result.username = ""sv;
        result.password = ""sv;
        result.hostname = ""sv;
        result.port = ""sv;
        result.pathname = ""sv;
        result.search = ""sv;
        result.hash = ""sv;
    }

    std::optional<upa::url> base_url = std::nullopt;
    if (init.base_url) {
        // construct and parse base_url
        base_url.emplace();
        if (!upa::success(base_url->parse(*init.base_url, nullptr)))
            throw urlpattern_error("invalid base URL");

        if (!init.protocol)
            result.protocol = process_base_url_string(base_url->get_part_view(upa::url::SCHEME), type);
        if (type != urlpattern_init_type::PATTERN &&
            !init.protocol && !init.hostname && !init.port && !init.username) {
            result.username = process_base_url_string(base_url->get_part_view(upa::url::USERNAME), type);
            if (!init.password)
                result.password = process_base_url_string(base_url->get_part_view(upa::url::PASSWORD), type);
        }
        if (!init.protocol && !init.hostname) {
            result.hostname = process_base_url_string(base_url->get_part_view(upa::url::HOST), type);
            if (!init.port) {
                result.port = process_base_url_string(base_url->get_part_view(upa::url::PORT), type);
                if (!init.pathname) {
                    result.pathname = process_base_url_string(base_url->get_part_view(upa::url::PATH), type);
                    if (!init.search) {
                        result.search = process_base_url_string(base_url->get_part_view(upa::url::QUERY), type);
                        if (!init.hash)
                            result.hash = process_base_url_string(base_url->get_part_view(upa::url::FRAGMENT), type);
                    }
                }
            }
        }
    }

    if (init.protocol)
        result.protocol = process_protocol_for_init(*init.protocol, type);
    if (init.username)
        result.username = process_username_for_init(*init.username, type);
    if (init.password)
        result.password = process_password_for_init(*init.password, type);
    if (init.hostname)
        result.hostname = process_hostname_for_init(*init.hostname, type);
    if (init.port)
        result.port = process_port_for_init(*init.port, result.protocol, type);
    if (init.pathname) {
        std::string new_pathname; // must have lifetime as result_pathname

        // Set result["pathname"] to init["pathname"]
        std::string_view result_pathname = *init.pathname;

        // If the following are all true:
        //  * baseURL is not null;
        //  * baseURL does not have an opaque path; and
        //  * the result of running is an absolute pathname given result["pathname"] and type is false,
        if (base_url && !base_url->has_opaque_path() &&
            !is_absolute_pathname(result_pathname, type))
        {
            const auto base_url_path = process_base_url_string(base_url->get_part_view(upa::url::PATH), type);
            // Let slash index be the index of the last U+002F (/) code point found in baseURLPath, interpreted
            // as a sequence of code points, or null if there are no instances of the code point.
            const auto slash_index = base_url_path.rfind('/'); // TODO: code point (not necessary)
            // If slash index is not null:
            if (slash_index != base_url_path.npos) {
                new_pathname = base_url_path.substr(0, slash_index + 1);
                new_pathname.append(result_pathname);
                result_pathname = new_pathname;
            }
        }
        result.pathname = process_pathname_for_init(result_pathname, result.protocol, type);
    }
    if (init.search)
        result.search = process_search_for_init(*init.search, type);
    if (init.hash)
        result.hash = process_hash_for_init(*init.hash, type);

    return result;
}

// https://urlpattern.spec.whatwg.org/#process-a-base-url-string
inline std::string process_base_url_string(std::string_view input, urlpattern_init_type type) {
    if (input.empty()) return {}; // MANO: optimization
    if (type != urlpattern_init_type::PATTERN)
        return std::string(input);
    return escape_pattern_string(input);
}

// https://urlpattern.spec.whatwg.org/#is-an-absolute-pathname
inline bool is_absolute_pathname(pattern_string_view input, urlpattern_init_type type) noexcept {
    if (input.empty()) return false;

    if (input[0] == '/')
        return true;
    if (type == urlpattern_init_type::URL)
        return false;

    // TODO: code point (not necessary)
    if (input.length() < 2)
        return false;
    if (input[0] == '\\' && input[1] == '/')
        return true;
    if (input[0] == '{' && input[1] == '/')
        return true;
    return false;
}

// https://urlpattern.spec.whatwg.org/#process-protocol-for-init
inline std::string process_protocol_for_init(std::string_view value, urlpattern_init_type type) {
    // Let strippedValue be the given value with a single trailing U+003A (:) removed, if any.
    const std::string_view stripped_value =
        (!value.empty() && value.back() == ':')
        ? value.substr(0, value.length() - 1)
        : value;
    if (type == urlpattern_init_type::PATTERN)
        return std::string{ stripped_value };
    return canonicalize_protocol(stripped_value);
}

// https://urlpattern.spec.whatwg.org/#process-username-for-init
inline std::string process_username_for_init(std::string_view value, urlpattern_init_type type) {
    if (type == urlpattern_init_type::PATTERN)
        return std::string{ value };
    return canonicalize_username(value);
}

// https://urlpattern.spec.whatwg.org/#process-password-for-init
inline std::string process_password_for_init(std::string_view value, urlpattern_init_type type) {
    if (type == urlpattern_init_type::PATTERN)
        return std::string{ value };
    return canonicalize_password(value);
}

// https://urlpattern.spec.whatwg.org/#process-hostname-for-init
inline std::string process_hostname_for_init(std::string_view value, urlpattern_init_type type) {
    if (type == urlpattern_init_type::PATTERN)
        return std::string{ value };
    return canonicalize_hostname(value);
}

// https://urlpattern.spec.whatwg.org/#process-port-for-init
inline std::string process_port_for_init(std::string_view port_value,
    std::optional<std::string_view> protocol_value, urlpattern_init_type type)
{
    if (type == urlpattern_init_type::PATTERN)
        return std::string{ port_value };
    return canonicalize_port(port_value, protocol_value);
}

// https://urlpattern.spec.whatwg.org/#process-pathname-for-init
inline std::string process_pathname_for_init(std::string_view pathname_value,
    std::optional<std::string_view> protocol_value, urlpattern_init_type type)
{
    if (type == urlpattern_init_type::PATTERN)
        return std::string{ pathname_value };
    // If protocolValue is a special scheme or the empty string, then return
    // the result of running canonicalize a pathname given pathnameValue.
    if (protocol_value && (protocol_value->empty() || is_special_scheme(*protocol_value)))
        return canonicalize_pathname(pathname_value);
    // Note
    // If the protocolValue is the empty string then no value was provided for protocol in the constructor
    // dictionary. Normally we do not special case empty string dictionary values, but in this case we
    // treat it as a special scheme in order to default to the most common pathname canonicalization.
    return canonicalize_opaque_pathname(pathname_value);
}

// https://urlpattern.spec.whatwg.org/#process-search-for-init
inline std::string process_search_for_init(std::string_view value, urlpattern_init_type type) {
    // Let strippedValue be the given value with a single leading U+003F (?) removed, if any.
    const std::string_view stripped_value =
        (!value.empty() && value.front() == '?')
        ? value.substr(1)
        : value;
    if (type == urlpattern_init_type::PATTERN)
        return std::string{ stripped_value };
    return canonicalize_search(stripped_value);
}

// https://urlpattern.spec.whatwg.org/#process-hash-for-init
inline std::string process_hash_for_init(std::string_view value, urlpattern_init_type type) {
    // Let strippedValue be the given value with a single leading U+0023 (#) removed, if any.
    const std::string_view stripped_value =
        (!value.empty() && value.front() == '#')
        ? value.substr(1)
        : value;
    if (type == urlpattern_init_type::PATTERN)
        return std::string{ stripped_value };
    return canonicalize_hash(stripped_value);
}


} // namespace upa::pattern

#endif // UPA_URLPATTERN_H
