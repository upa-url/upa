// Copyright 2016-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
// This file contains portions of modified code from:
// https://cs.chromium.org/chromium/src/url/url_canon_etc.cc
// Copyright 2013 The Chromium Authors. All rights reserved.
//

// URL Standard
// https://url.spec.whatwg.org/
//
// Infra Standard - fundamental concepts upon which standards are built
// https://infra.spec.whatwg.org/
//

#ifndef UPA_URL_H
#define UPA_URL_H

#include "buffer.h"
#include "config.h"
#include "str_arg.h"            // IWYU pragma: export
#include "url_host.h"           // IWYU pragma: export
#include "url_percent_encode.h" // IWYU pragma: export
#include "url_result.h"         // IWYU pragma: export
#include "url_search_params.h"  // IWYU pragma: export
#include "url_version.h"        // IWYU pragma: export
#include "util.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint> // uint8_t
#include <functional> // std::hash
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// not yet
// #define UPA_URL_USE_ENCODING

namespace upa {

// forward declarations

namespace detail {
    class url_serializer;
    class url_setter;
    class url_parser;

    // url_error what() values
    extern const char* const kURLParseError;
    extern const char* const kBaseURLParseError;
} // namespace detail

/// @brief URL class
///
/// Follows specification in
/// https://url.spec.whatwg.org/#url-class
///
class url {
public:
    /// Enumeration to identify URL's parts (URL record members) defined here:
    /// https://url.spec.whatwg.org/#url-representation
    enum PartType {
        SCHEME = 0,
        SCHEME_SEP,
        USERNAME,
        PASSWORD,
        HOST_START,
        HOST,
        PORT,
        PATH_PREFIX,    // "/."
        PATH,
        QUERY,
        FRAGMENT,
        PART_COUNT
    };

    /// @brief Default constructor.
    ///
    /// Constructs empty URL.
    url() = default;

    /// @brief Copy constructor.
    ///
    /// @param[in] other  URL to copy from
    url(const url& other) = default;

    /// @brief Move constructor.
    ///
    /// Constructs the URL with the contents of @a other using move semantics.
    ///
    /// @param[in,out] other  URL to move to this object
    url(url&& other) noexcept;

    /// @brief Copy assignment.
    ///
    /// @param[in] other  URL to copy from
    /// @return *this
    url& operator=(const url& other) = default;

    /// @brief Move assignment.
    ///
    /// Replaces the contents with those of @a other using move semantics.
    ///
    /// @param[in,out] other  URL to move to this object
    /// @return *this
    url& operator=(url&& other) UPA_NOEXCEPT_17;

    /// @brief Safe move assignment.
    ///
    /// Replaces the contents with those of @a other using move semantics.
    /// Preserves original url_search_params object.
    ///
    /// @param[in,out] other  URL to move to this object
    /// @return *this
    url& safe_assign(url&& other);

    /// @brief Parsing constructor.
    ///
    /// Throws url_error exception on parse error.
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] pbase   pointer to base URL, may be `nullptr`
    template <class T, enable_if_str_arg_t<T> = 0>
    explicit url(T&& str_url, const url* pbase = nullptr)
        : url(std::forward<T>(str_url), pbase, detail::kURLParseError)
    {}

    /// @brief Parsing constructor.
    ///
    /// Throws url_error exception on parse error.
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] base    base URL
    template <class T, enable_if_str_arg_t<T> = 0>
    explicit url(T&& str_url, const url& base)
        : url(std::forward<T>(str_url), &base, detail::kURLParseError)
    {}

    /// @brief Parsing constructor.
    ///
    /// Throws url_error exception on parse error.
    ///
    /// @param[in] str_url  URL string to parse
    /// @param[in] str_base base URL string
    template <class T, class TB, enable_if_str_arg_t<T> = 0, enable_if_str_arg_t<TB> = 0>
    explicit url(T&& str_url, TB&& str_base)
        : url(std::forward<T>(str_url), url(std::forward<TB>(str_base), nullptr, detail::kBaseURLParseError))
    {}

    /// destructor
    ~url() = default;

    // Operations

    /// @brief Clears URL
    ///
    /// Makes URL empty.
    void clear();

    /// @brief Swaps the contents of two URLs
    ///
    /// @param[in,out] other URL to exchange the contents with
    void swap(url& other) UPA_NOEXCEPT_17;

    // Parser

    /// @brief Parses given URL string against base URL.
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] base    pointer to base URL, may be nullptr
    /// @return error code (@a validation_errc::ok on success)
    template <class T, enable_if_str_arg_t<T> = 0>
    validation_errc parse(T&& str_url, const url* base = nullptr) {
        const auto inp = make_str_arg(std::forward<T>(str_url));
        return do_parse(inp.begin(), inp.end(), base);
    }

    /// @brief Parses given URL string against base URL.
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] base    base URL
    /// @return error code (@a validation_errc::ok on success)
    template <class T, enable_if_str_arg_t<T> = 0>
    validation_errc parse(T&& str_url, const url& base) {
        return parse(std::forward<T>(str_url), &base);
    }

    /// @brief Parses given URL string against base URL.
    ///
    /// @param[in] str_url  URL string to parse
    /// @param[in] str_base base URL string
    /// @return error code (@a validation_errc::ok on success)
    template <class T, class TB, enable_if_str_arg_t<T> = 0, enable_if_str_arg_t<TB> = 0>
    validation_errc parse(T&& str_url, TB&& str_base) {
        upa::url base;
        const auto res = base.parse(std::forward<TB>(str_base), nullptr);
        return res == validation_errc::ok
            ? parse(std::forward<T>(str_url), &base)
            : res;
    }

    /// @brief Checks if a given URL string can be successfully parsed
    ///
    /// If @a pbase is not nullptr, then try to parse against *pbase URL.
    /// More info: https://url.spec.whatwg.org/#dom-url-canparse
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] pbase   pointer to base URL, may be `nullptr`
    /// @return true if given @a str_url can be parsed against @a *pbase
    template <class T, enable_if_str_arg_t<T> = 0>
    static bool can_parse(T&& str_url, const url* pbase = nullptr) {
        upa::url url;
        return url.for_can_parse(std::forward<T>(str_url), pbase) == validation_errc::ok;
    }

    /// @brief Checks if a given URL string can be successfully parsed
    ///
    /// Try to parse against base URL.
    /// More info: https://url.spec.whatwg.org/#dom-url-canparse
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] base    base URL
    /// @return true if given @a str_url can be parsed against base URL
    template <class T, enable_if_str_arg_t<T> = 0>
    static bool can_parse(T&& str_url, const url& base) {
        return can_parse(std::forward<T>(str_url), &base);
    }

    /// @brief Checks if a given URL string can be successfully parsed
    ///
    /// First try to parse @a str_base URL string, if it succeed, then
    /// try to parse @a str_url against base URL.
    /// More info: https://url.spec.whatwg.org/#dom-url-canparse
    ///
    /// @param[in] str_url  URL string to parse
    /// @param[in] str_base base URL string
    /// @return true if given @a str_url can be parsed against @a str_base URL string
    template <class T, class TB, enable_if_str_arg_t<T> = 0, enable_if_str_arg_t<TB> = 0>
    static bool can_parse(T&& str_url, TB&& str_base) {
        upa::url base;
        return
            base.for_can_parse(std::forward<TB>(str_base), nullptr) == validation_errc::ok &&
            can_parse(std::forward<T>(str_url), &base);
    }

    // Setters

    /// @brief The href setter
    ///
    /// Parses given URL string, and in the case of success assigns parsed URL value.
    /// On parse failure leaves URL value unchanged.
    ///
    /// @param[in] str URL string to parse
    /// @return `true` - on success; `false` - on failure
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool href(StrT&& str);
    /// Equivalent to @link href(StrT&& str) @endlink
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool set_href(StrT&& str) { return href(std::forward<StrT>(str)); }

    /// @brief The protocol setter
    ///
    /// Parses given string and on succes sets the URL's protocol.
    /// More info: https://url.spec.whatwg.org/#dom-url-protocol
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL protocol unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool protocol(StrT&& str);
    /// Equivalent to @link protocol(StrT&& str) @endlink
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool set_protocol(StrT&& str) { return protocol(std::forward<StrT>(str)); }

    /// @brief The username setter
    ///
    /// Parses given string and on succes sets the URL's username.
    /// More info: https://url.spec.whatwg.org/#dom-url-username
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - if username can not be set
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool username(StrT&& str);
    /// Equivalent to @link username(StrT&& str) @endlink
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool set_username(StrT&& str) { return username(std::forward<StrT>(str)); }

    /// @brief The password setter
    ///
    /// Parses given string and on succes sets the URL's password.
    /// More info: https://url.spec.whatwg.org/#dom-url-password
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - if password can not be set
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool password(StrT&& str);
    /// Equivalent to @link password(StrT&& str) @endlink
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool set_password(StrT&& str) { return password(std::forward<StrT>(str)); }

    /// @brief The host setter
    ///
    /// Parses given string and on succes sets the URL's host and port.
    /// More info: https://url.spec.whatwg.org/#dom-url-host
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's host and port unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool host(StrT&& str);
    /// Equivalent to @link host(StrT&& str) @endlink
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool set_host(StrT&& str) { return host(std::forward<StrT>(str)); }

    /// @brief The hostname setter
    ///
    /// Parses given string and on succes sets the URL's host.
    /// More info: https://url.spec.whatwg.org/#dom-url-hostname
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's host unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool hostname(StrT&& str);
    /// Equivalent to @link hostname(StrT&& str) @endlink
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool set_hostname(StrT&& str) { return hostname(std::forward<StrT>(str)); }

    /// @brief The port setter
    ///
    /// Parses given string and on succes sets the URL's port.
    /// More info: https://url.spec.whatwg.org/#dom-url-port
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's port unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool port(StrT&& str);
    /// Equivalent to @link port(StrT&& str) @endlink
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool set_port(StrT&& str) { return port(std::forward<StrT>(str)); }

    /// @brief The pathname setter
    ///
    /// Parses given string and on succes sets the URL's path.
    /// More info: https://url.spec.whatwg.org/#dom-url-pathname
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's path unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool pathname(StrT&& str);
    /// Equivalent to @link pathname(StrT&& str) @endlink
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool set_pathname(StrT&& str) { return pathname(std::forward<StrT>(str)); }

    /// @brief The search setter
    ///
    /// Parses given string and on succes sets the URL's query.
    /// More info: https://url.spec.whatwg.org/#dom-url-search
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's query unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool search(StrT&& str);
    /// Equivalent to @link search(StrT&& str) @endlink
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool set_search(StrT&& str) { return search(std::forward<StrT>(str)); }

    /// @brief The hash setter
    ///
    /// Parses given string and on succes sets the URL's fragment.
    /// More info: https://url.spec.whatwg.org/#dom-url-hash
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's fragment unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool hash(StrT&& str);
    /// Equivalent to @link hash(StrT&& str) @endlink
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool set_hash(StrT&& str) { return hash(std::forward<StrT>(str)); }

    // Getters

    /// @brief The href getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-href
    ///
    /// @return serialized URL
    string_view href() const;
    /// Equivalent to @link href() const @endlink
    string_view get_href() const { return href(); }

    /// @brief The origin getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-origin
    ///
    /// Note: For file URLs, this implementation returns a serialized opaque
    /// origin (null).
    ///
    /// @return ASCII serialized URL's origin
    std::string origin() const;

    /// @brief The protocol getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-protocol
    ///
    /// @return URL's scheme, followed by U+003A (:)
    string_view protocol() const;
    /// Equivalent to @link protocol() const @endlink
    string_view get_protocol() const { return protocol(); }

    /// @brief The username getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-username
    ///
    /// @return URL’s username
    string_view username() const;
    /// Equivalent to @link username() const @endlink
    string_view get_username() const { return username(); }

    /// @brief The password getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-password
    ///
    /// @return URL’s password
    string_view password() const;
    /// Equivalent to @link password() const @endlink
    string_view get_password() const { return password(); }

    /// @brief The host getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-host
    ///
    /// @return URL’s host, serialized, followed by U+003A (:) and URL’s port, serialized
    string_view host() const;
    /// Equivalent to @link host() const @endlink
    string_view get_host() const { return host(); }

    /// @brief The hostname getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-hostname
    ///
    /// @return URL’s host, serialized
    string_view hostname() const;
    /// Equivalent to @link hostname() const @endlink
    string_view get_hostname() const { return hostname(); }

    /// @brief The host_type getter
    ///
    /// @return URL’s host type as HostType enumeration value
    HostType host_type() const noexcept;

    /// @brief The port getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-port
    ///
    /// @return URL’s port, serialized, if URL’s port is not null, otherwise empty string
    string_view port() const;
    /// Equivalent to @link port() const @endlink
    string_view get_port() const { return port(); }

    /// @return URL’s port, converted to `int` value, if URL’s port is not null,
    ///   otherwise `-1`
    int port_int() const;

    /// @return URL’s port, converted to `int` value, if URL’s port is not null,
    ///   otherwise default port, if URL's scheme has default port,
    ///   otherwise `-1`
    int real_port_int() const;

    /// @brief The path getter
    ///
    /// @return URL's path, serialized, followed by U+003F (?) and URL’s query
    string_view path() const;
    /// Equivalent to @link path() const @endlink
    string_view get_path() const { return path(); }

    /// @brief The pathname getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-pathname
    ///
    /// @return URL’s path, serialized
    string_view pathname() const;
    /// Equivalent to @link pathname() const @endlink
    string_view get_pathname() const { return pathname(); }

    /// @brief The search getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-search
    ///
    /// @return empty string or U+003F (?), followed by URL’s query
    string_view search() const;
    /// Equivalent to @link search() const @endlink
    string_view get_search() const { return search(); }

    /// @brief The hash getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-hash
    ///
    /// @return empty string or U+0023 (#), followed by URL’s fragment
    string_view hash() const;
    /// Equivalent to @link hash() const @endlink
    string_view get_hash() const { return hash(); }

    /// @brief The searchParams getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-searchparams
    ///
    /// Returned reference is valid thru lifetime of url, or until url's move assignment
    /// operation (except @c safe_assign, which preserves reference validity).
    ///
    /// @return reference to this’s query object (url_search_params class)
    url_search_params& search_params()&;

    /// @brief The searchParams getter for rvalue url
    ///
    /// @return the move constructed copy of this’s query object (url_search_params class)
    /// @see search_params()&
    url_search_params search_params()&&;

    /// @brief URL serializer
    ///
    /// Returns serialized URL in a string_view as defined here:
    /// https://url.spec.whatwg.org/#concept-url-serializer
    ///
    /// @param[in] exclude_fragment exclude fragment when serializing
    /// @return serialized URL as string_view
    string_view serialize(bool exclude_fragment = false) const;

    // Get url info

    /// @brief Checks whether the URL is empty
    ///
    /// @return `true` if URL is empty, `false` otherwise
    bool empty() const noexcept;

    /// @brief Returns whether the URL is valid
    ///
    /// URL is valid if it is not empty, and contains a successfully parsed URL.
    ///
    /// @return `true` if URL is valid, `false` otherwise
    bool is_valid() const noexcept;

    /// @brief Gets URL's part (URL record member) as string
    ///
    /// Function to get ASCII string of any URL's part (URL record member) defined here:
    /// https://url.spec.whatwg.org/#url-representation
    ///
    /// * `get_part_view(upa::url::SCHEME)` - get a URL’s **scheme** string
    /// * `get_part_view(upa::url::USERNAME)` - get a URL’s **username** string
    /// * `get_part_view(upa::url::PASSWORD)` - get a URL’s **password** string
    /// * `get_part_view(upa::url::HOST)` - get a URL’s **host** serialized to string
    /// * `get_part_view(upa::url::PORT)` - get a URL’s **port** serialized to string
    /// * `get_part_view(upa::url::PATH)` - get a URL’s **path** serialized to string
    /// * `get_part_view(upa::url::QUERY)` - get a URL’s **query** string
    /// * `get_part_view(upa::url::FRAGMENT)` - get a URL’s **fragment** string
    ///
    /// @param[in] t URL's part
    /// @return URL's part string; it is empty if part is empty or null
    string_view get_part_view(PartType t) const;

    /// @brief Checks whether the URL's part (URL record member) is empty or null
    ///
    /// @param[in] t URL's part
    /// @return `true` if URL's part @a t is empty or null, `false` otherwise
    bool is_empty(PartType t) const;

    /// @brief Checks whether the URL's part (URL record member) is null
    ///
    /// Only the following [URL record members](https://url.spec.whatwg.org/#concept-url)
    /// can be null:
    /// * **host** - check with `is_null(upa::url::HOST)`
    /// * **port** - check with `is_null(upa::url::PORT)`
    /// * **query** - check with `is_null(upa::url::QUERY)`
    /// * **fragment** - check with `is_null(upa::url::FRAGMENT)`
    ///
    /// @param[in] t URL's part
    /// @return `true` if URL's part @a t is null, `false` otherwise
    bool is_null(PartType t) const noexcept;

    /// @return `true` if URL's scheme is special ("ftp", "file", "http", "https", "ws", or "wss"),
    ///   `false` otherwise; see: https://url.spec.whatwg.org/#special-scheme
    bool is_special_scheme() const noexcept;

    /// @return `true` if URL's scheme is "file", `false` otherwise
    bool is_file_scheme() const noexcept;

    /// @return `true` if URL's scheme is "http" or "https", `false` otherwise
    bool is_http_scheme() const noexcept;

    /// @return `true` if URL includes credentials (username, password), `false` otherwise
    bool has_credentials() const;

    /// see: https://url.spec.whatwg.org/#url-opaque-path
    /// @return `true` if URL's path is a URL path segment, `false` otherwise
    bool has_opaque_path() const noexcept;

    /// @return serialized URL as `std::string`
    std::string to_string() const;

private:
    struct alignas(32) scheme_info {
        string_view scheme;
        int default_port;           // -1 if none
        unsigned is_special : 1;    // "ftp", "file", "http", "https", "ws", "wss"
        unsigned is_file : 1;       // "file"
        unsigned is_http : 1;       // "http", "https"
        unsigned is_ws : 1;         // "ws", "wss"
    };

    enum UrlFlag : unsigned {
        // not null flags
        SCHEME_FLAG = (1u << SCHEME),
        USERNAME_FLAG = (1u << USERNAME),
        PASSWORD_FLAG = (1u << PASSWORD),
        HOST_FLAG = (1u << HOST),
        PORT_FLAG = (1u << PORT),
        PATH_FLAG = (1u << PATH),
        QUERY_FLAG = (1u << QUERY),
        FRAGMENT_FLAG = (1u << FRAGMENT),
        // other flags
        OPAQUE_PATH_FLAG = (1u << (PART_COUNT + 0)),
        VALID_FLAG = (1u << (PART_COUNT + 1)),
        // host type
        HOST_TYPE_SHIFT = (PART_COUNT + 2),
        HOST_TYPE_MASK = (7u << HOST_TYPE_SHIFT),

        // initial flags (empty (but not null) parts)
        // https://url.spec.whatwg.org/#url-representation
        INITIAL_FLAGS = SCHEME_FLAG | USERNAME_FLAG | PASSWORD_FLAG | PATH_FLAG,
    };

    static const unsigned kPartFlagMask[url::PART_COUNT];

    // parsing constructor
    template <class T, enable_if_str_arg_t<T> = 0>
    explicit url(T&& str_url, const url* base, const char* what_arg);

    // parser
    template <typename CharT>
    validation_errc do_parse(const CharT* first, const CharT* last, const url* base);

    template <class T, enable_if_str_arg_t<T> = 0>
    validation_errc for_can_parse(T&& str_url, const url* base);

    // get scheme info
    static const scheme_info kSchemes[];
    static const scheme_info* get_scheme_info(string_view src);

    // set scheme
    template <class StringT>
    void set_scheme_str(StringT str);
    void set_scheme(const url& src);
    void set_scheme(string_view str);
    void set_scheme(std::size_t scheme_length);

    // path util
    string_view get_path_first_string(std::size_t len) const;
    // path shortening
    bool get_path_rem_last(std::size_t& path_end, std::size_t& path_segment_count) const;
    bool get_shorten_path(std::size_t& path_end, std::size_t& path_segment_count) const;

    // flags
    void set_flag(UrlFlag flag) noexcept;

    void set_has_opaque_path() noexcept;

    void set_host_type(HostType ht) noexcept;

    // info
    bool canHaveUsernamePasswordPort() const;

    // url record
    void move_record(url& other) UPA_NOEXCEPT_17;

    // search params
    void clear_search_params() noexcept;
    void parse_search_params();

private:
    std::string norm_url_;
    std::array<std::size_t, PART_COUNT> part_end_ = {};
    const scheme_info* scheme_inf_ = nullptr;
    unsigned flags_ = INITIAL_FLAGS;
    std::size_t path_segment_count_ = 0;
    detail::url_search_params_ptr search_params_ptr_;

    friend bool operator==(const url& lhs, const url& rhs) noexcept;
    friend struct std::hash<url>;
    friend class detail::url_serializer;
    friend class detail::url_setter;
    friend class detail::url_parser;
    friend class url_search_params;
};


namespace detail {

class url_serializer : public host_output {
public:
    url_serializer() = delete;
    url_serializer(const url_serializer&) = delete;
    url_serializer& operator=(const url_serializer&) = delete;

    explicit url_serializer(url& dest_url, bool need_save = true)
        : host_output(need_save)
        , url_(dest_url)
        , last_pt_(url::SCHEME)
    {}

    ~url_serializer() override = default;

    void new_url() {
        if (!url_.empty())
            url_.clear();
    }
    virtual void reserve(std::size_t new_cap) { url_.norm_url_.reserve(new_cap); }

    // set data
    void set_scheme(const url& src) { url_.set_scheme(src); }
    void set_scheme(const string_view str) { url_.set_scheme(str); }
    void set_scheme(std::size_t scheme_length) { url_.set_scheme(scheme_length); }

    // set scheme
    virtual std::string& start_scheme();
    virtual void save_scheme();

    // set url's part
    void fill_parts_offset(url::PartType t1, url::PartType t2, std::size_t offset);
    virtual std::string& start_part(url::PartType new_pt);
    virtual void save_part();

    virtual void clear_part(url::PartType /*pt*/) {}

    // set empty host
    void set_empty_host();

    // empties not empty host
    virtual void empty_host();

    // host_output overrides
    std::string& hostStart() override;
    void hostDone(HostType ht) override;

    // Path operations

    // append the empty string to url’s path (list)
    void append_empty_path_segment();
    // append string to url's path (list)
    virtual std::string& start_path_segment();
    virtual void save_path_segment();
    virtual void commit_path();
    // if '/' not required:
    std::string& start_path_string();
    void save_path_string();

    virtual void shorten_path();
    //UNUSED// retunrs how many slashes are removed
    //virtual std::size_t remove_leading_path_slashes();

    using PathOpFn = bool (url::*)(std::size_t& path_end, std::size_t& segment_count) const;
    void append_parts(const url& src, url::PartType t1, url::PartType t2, PathOpFn pathOpFn = nullptr);

    // flags
    void set_flag(const url::UrlFlag flag) { url_.set_flag(flag); }
    void set_host_type(const HostType ht) { url_.set_host_type(ht); }
    // IMPORTANT: has-an-opaque-path flag must be set before or just after
    // SCHEME set; because other part's serialization depends on this flag
    void set_has_opaque_path() {
        assert(last_pt_ == url::SCHEME);
        url_.set_has_opaque_path();
    }

    // get info
    string_view get_part_view(url::PartType t) const { return url_.get_part_view(t); }
    bool is_empty(const url::PartType t) const { return url_.is_empty(t); }
    virtual bool is_empty_path() const {
        assert(!url_.has_opaque_path());
        // path_segment_count_ has meaning only if path is a list (path isn't opaque)
        return url_.path_segment_count_ == 0;
    }
    bool is_null(const url::PartType t) const noexcept { return url_.is_null(t); }
    bool is_special_scheme() const noexcept { return url_.is_special_scheme(); }
    bool is_file_scheme() const noexcept { return url_.is_file_scheme(); }
    bool has_credentials() const { return url_.has_credentials(); }
    const url::scheme_info* scheme_inf() const noexcept { return url_.scheme_inf_; }
    int port_int() const { return url_.port_int(); }

protected:
    void adjust_path_prefix();

    std::size_t get_part_pos(url::PartType pt) const;
    std::size_t get_part_len(url::PartType pt) const;
    void replace_part(url::PartType new_pt, const char* str, std::size_t len);
    void replace_part(url::PartType last_pt, const char* str, std::size_t len,
        url::PartType first_pt, std::size_t len0);

protected:
    url& url_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    // last serialized URL's part
    url::PartType last_pt_;
};


class url_setter : public url_serializer {
public:
    url_setter() = delete;
    url_setter(const url_setter&) = delete;
    url_setter& operator=(const url_setter&) = delete;

    explicit url_setter(url& dest_url)
        : url_serializer(dest_url)
        , use_strp_(true)
        , curr_pt_(url::SCHEME)
    {}

    ~url_setter() override = default;

    //???
    void reserve(std::size_t new_cap) override;

    // set scheme
    std::string& start_scheme() override;
    void save_scheme() override;

    // set/clear/empty url's part
    std::string& start_part(url::PartType new_pt) override;
    void save_part() override;

    void clear_part(url::PartType pt) override;
    void empty_part(url::PartType pt); // override

    void empty_host() override;

    // path
    std::string& start_path_segment() override;
    void save_path_segment() override;
    void commit_path() override;

    void shorten_path() override;
    //UNUSED// retunrs how many slashes are removed
    //std::size_t remove_leading_path_slashes() override;
    bool is_empty_path() const override;

    void potentially_strip_trailing_spaces_from_an_opaque_path();

protected:
    url::PartType find_last_part(url::PartType pt) const;

private:
    bool use_strp_;
    // buffer for URL's part
    std::string strp_;
    // path segment end positions in the strp_
    std::vector<std::size_t> path_seg_end_;
    // current URL's part
    url::PartType curr_pt_;
};


class url_parser {
public:
    enum State {
        not_set_state = 0,
        scheme_start_state,
        scheme_state,
        no_scheme_state,
        special_relative_or_authority_state,
        path_or_authority_state,
        relative_state,
        relative_slash_state,
        special_authority_slashes_state,
        special_authority_ignore_slashes_state,
        authority_state,
        host_state,
        hostname_state,
        port_state,
        file_state,
        file_slash_state,
        file_host_state,
        path_start_state,
        path_state,
        opaque_path_state,
        query_state,
        fragment_state
    };

    template <typename CharT>
    static validation_errc url_parse(url_serializer& urls, const CharT* first, const CharT* last, const url* base, State state_override = not_set_state);

    template <typename CharT>
    static validation_errc parse_host(url_serializer& urls, const CharT* first, const CharT* last);

    template <typename CharT>
    static void parse_path(url_serializer& urls, const CharT* first, const CharT* last);

private:
    template <typename CharT>
    static bool do_path_segment(const CharT* pointer, const CharT* last, std::string& output);

    template <typename CharT>
    static bool do_simple_path(const CharT* pointer, const CharT* last, std::string& output);
};


// part start
extern const uint8_t kPartStart[url::PART_COUNT];

inline int port_from_str(const char* first, const char* last) noexcept {
    int port = 0;
    for (auto it = first; it != last; ++it) {
        port = port * 10 + (*it - '0');
    }
    return port;
}

// Removable URL chars

// chars to trim (C0 control or space: U+0000 to U+001F or U+0020)
template <typename CharT>
constexpr bool is_trim_char(CharT ch) noexcept {
    return util::to_unsigned(ch) <= ' ';
}

// chars what should be removed from the URL (ASCII tab or newline: U+0009, U+000A, U+000D)
template <typename CharT>
constexpr bool is_removable_char(CharT ch) noexcept {
    return ch == '\r' || ch == '\n' || ch == '\t';
}

template <typename CharT>
inline void do_trim(const CharT*& first, const CharT*& last) noexcept {
    // remove leading C0 controls and space
    while (first < last && is_trim_char(*first))
        ++first;
    // remove trailing C0 controls and space
    while (first < last && is_trim_char(*(last-1)))
        --last;
}

// DoRemoveURLWhitespace
// https://cs.chromium.org/chromium/src/url/url_canon_etc.cc
template <typename CharT>
inline void do_remove_whitespace(const CharT*& first, const CharT*& last, simple_buffer<CharT>& buff) {
    // Fast verification that there's nothing that needs removal. This is the 99%
    // case, so we want it to be fast and don't care about impacting the speed
    // when we do find whitespace.
    for (auto it = first; it < last; ++it) {
        if (!is_removable_char(*it))
            continue;
        // copy non whitespace chars into the new buffer and return it
        buff.reserve(last - first);
        buff.append(first, it);
        for (; it < last; ++it) {
            if (!is_removable_char(*it))
                buff.push_back(*it);
        }
        first = buff.data();
        last = buff.data() + buff.size();
        break;
    }
}

// reverse find

template<class InputIt, class T>
inline InputIt find_last(InputIt first, InputIt last, const T& value) {
    for (auto it = last; it > first;) {
        --it;
        if (*it == value) return it;
    }
    return last;
}

// special chars

template <typename CharT>
constexpr bool is_slash(CharT ch) noexcept {
    return ch == '/' || ch == '\\';
}

template <typename CharT>
constexpr bool is_posix_slash(CharT ch) noexcept {
    return ch == '/';
}

template <typename CharT>
constexpr bool is_windows_slash(CharT ch) noexcept {
    return ch == '\\' || ch == '/';
}

// Scheme chars

template <typename CharT>
constexpr bool is_first_scheme_char(CharT ch) noexcept {
    return is_ascii_alpha(ch);
}

template <typename CharT>
constexpr bool is_authority_end_char(CharT c) noexcept {
    return c == '/' || c == '?' || c == '#';
}

template <typename CharT>
constexpr bool is_special_authority_end_char(CharT c) noexcept {
    return c == '/' || c == '?' || c == '#' || c == '\\';
}

// Windows drive letter

// https://url.spec.whatwg.org/#windows-drive-letter
template <typename CharT>
constexpr bool is_windows_drive(CharT c1, CharT c2) noexcept {
    return is_ascii_alpha(c1) && (c2 == ':' || c2 == '|');
}

// https://url.spec.whatwg.org/#normalized-windows-drive-letter
template <typename CharT>
constexpr bool is_normalized_windows_drive(CharT c1, CharT c2) noexcept {
    return is_ascii_alpha(c1) && c2 == ':';
}

// https://url.spec.whatwg.org/#start-with-a-windows-drive-letter
template <typename CharT>
inline bool starts_with_windows_drive(const CharT* pointer, const CharT* last) noexcept {
    const auto length = last - pointer;
    return
        (length == 2 || (length > 2 && detail::is_special_authority_end_char(pointer[2]))) &&
        detail::is_windows_drive(pointer[0], pointer[1]);
/*** alternative implementation ***
    return
        length >= 2 &&
        detail::is_windows_drive(pointer[0], pointer[1]) &&
        (length == 2 || detail::is_special_authority_end_char(pointer[2]));
***/
}

// Windows drive letter in OS path
//
// NOTE: Windows OS supports only normalized Windows drive letters.

// Check url's pathname has Windows drive, i.e. starts with "/C:/" or is "/C:"
// see also: detail::starts_with_windows_drive
inline bool pathname_has_windows_os_drive(string_view pathname) noexcept {
    return
        (pathname.length() == 3 || (pathname.length() > 3 && is_windows_slash(pathname[3]))) &&
        is_windows_slash(pathname[0]) &&
        is_normalized_windows_drive(pathname[1], pathname[2]);
}

/// Check string is absolute Windows drive path (for example: "C:\\path" or "C:/path")
/// @return pointer to the path after first (back)slash, or `nullptr` if path is not
///   absolute Windows drive path
template <typename CharT>
constexpr const CharT* is_windows_os_drive_absolute_path(const CharT* pointer, const CharT* last) noexcept {
    return (last - pointer > 2 &&
        is_normalized_windows_drive(pointer[0], pointer[1]) &&
        is_windows_slash(pointer[2]))
        ? pointer + 3 : nullptr;
}

} // namespace detail


// url class

inline url::url(url&& other) noexcept
    : norm_url_(std::move(other.norm_url_))
    , part_end_(other.part_end_)
    , scheme_inf_(other.scheme_inf_)
    , flags_(other.flags_)
    , path_segment_count_(other.path_segment_count_)
    , search_params_ptr_(std::move(other.search_params_ptr_))
{
    search_params_ptr_.set_url_ptr(this);
}

inline url& url::operator=(url&& other) UPA_NOEXCEPT_17 {
    // move data
    move_record(other);
    search_params_ptr_ = std::move(other.search_params_ptr_);

    // setup search params
    search_params_ptr_.set_url_ptr(this);

    return *this;
}

inline url& url::safe_assign(url&& other) {
    if (search_params_ptr_) {
        if (other.search_params_ptr_) {
            move_record(other);
            search_params_ptr_->move_params(std::move(*other.search_params_ptr_));
        } else {
            // parse search parameters before move assign for strong exception guarantee
            url_search_params params(&other);
            move_record(other);
            search_params_ptr_->move_params(std::move(params));
        }
    } else {
        move_record(other);
    }
    return *this;
}

inline void url::move_record(url& other) UPA_NOEXCEPT_17 {
    norm_url_ = std::move(other.norm_url_);
    part_end_ = other.part_end_;
    scheme_inf_ = other.scheme_inf_;
    flags_ = other.flags_;
    path_segment_count_ = other.path_segment_count_;
}

// url getters

inline string_view url::href() const {
    return norm_url_;
}

inline std::string url::to_string() const {
    return norm_url_;
}

// Origin
// https://url.spec.whatwg.org/#concept-url-origin

// ASCII serialization of an origin
// https://html.spec.whatwg.org/multipage/browsers.html#ascii-serialisation-of-an-origin
inline std::string url::origin() const {
    if (is_special_scheme()) {
        if (is_file_scheme())
            return "null"; // opaque origin
        // "scheme://"
        std::string str_origin(norm_url_, 0, part_end_[SCHEME_SEP]);
        // "host:port"
        str_origin.append(norm_url_.data() + part_end_[HOST_START], norm_url_.data() + part_end_[PORT]);
        return str_origin;
    }
    if (get_part_view(SCHEME) == string_view{ "blob", 4 }) {
        // Note: this library does not support blob URL store, so it allways assumes
        // URL's blob URL entry is null and retrieves origin from the URL's path.
        url path_url;
        if (path_url.parse(get_part_view(PATH)) == validation_errc::ok &&
            path_url.is_http_scheme())
            return path_url.origin();
    }
    return "null"; // opaque origin
}

inline string_view url::protocol() const {
    // "scheme:"
    return { norm_url_.data(), part_end_[SCHEME] ? part_end_[SCHEME] + 1 : 0 };
}

inline string_view url::username() const {
    return get_part_view(USERNAME);
}

inline string_view url::password() const {
    return get_part_view(PASSWORD);
}

inline string_view url::host() const {
    if (is_null(HOST))
        return {};
    // "hostname:port"
    const std::size_t b = part_end_[HOST_START];
    const std::size_t e = is_null(PORT) ? part_end_[HOST] : part_end_[PORT];
    return { norm_url_.data() + b, e - b };
}

inline string_view url::hostname() const {
    return get_part_view(HOST);
}

inline HostType url::host_type() const noexcept {
    return static_cast<HostType>((flags_ & HOST_TYPE_MASK) >> HOST_TYPE_SHIFT);
}

inline string_view url::port() const {
    return get_part_view(PORT);
}

inline int url::port_int() const {
    const auto vport = get_part_view(PORT);
    return vport.length() ? detail::port_from_str(vport.data(), vport.data() + vport.length()) : -1;
}

inline int url::real_port_int() const {
    const auto vport = get_part_view(PORT);
    if (vport.length())
        return detail::port_from_str(vport.data(), vport.data() + vport.length());
    return scheme_inf_ ? scheme_inf_->default_port : -1;
}

// pathname + search
inline string_view url::path() const {
    // "pathname?query"
    const std::size_t b = part_end_[PATH - 1];
    const std::size_t e = part_end_[QUERY] ? part_end_[QUERY] : part_end_[PATH];
    return { norm_url_.data() + b, e ? e - b : 0 };
}

inline string_view url::pathname() const {
    // https://url.spec.whatwg.org/#dom-url-pathname
    // already serialized as needed
    return get_part_view(PATH);
}

inline string_view url::search() const {
    const std::size_t b = part_end_[QUERY - 1];
    const std::size_t e = part_end_[QUERY];
    // is empty?
    if (b + 1 >= e)
        return {};
    // return with '?'
    return { norm_url_.data() + b, e - b };
}

inline string_view url::hash() const {
    const std::size_t b = part_end_[FRAGMENT - 1];
    const std::size_t e = part_end_[FRAGMENT];
    // is empty?
    if (b + 1 >= e)
        return {};
    // return with '#'
    return { norm_url_.data() + b, e - b };
}

inline url_search_params& url::search_params()& {
    if (!search_params_ptr_)
        search_params_ptr_.init(this);
    return *search_params_ptr_;
}

inline url_search_params url::search_params()&& {
    if (search_params_ptr_)
        return std::move(*search_params_ptr_);
    return url_search_params{ search() };
}

inline void url::clear_search_params() noexcept {
    if (search_params_ptr_)
        search_params_ptr_.clear_params();
}

inline void url::parse_search_params() {
    if (search_params_ptr_)
        search_params_ptr_.parse_params(get_part_view(QUERY));
}

inline string_view url::serialize(bool exclude_fragment) const {
    if (exclude_fragment && part_end_[FRAGMENT])
        return { norm_url_.data(), part_end_[QUERY] };
    return norm_url_;
}

// Get url info

inline bool url::empty() const noexcept {
    return norm_url_.empty();
}

inline bool url::is_valid() const noexcept {
    return !!(flags_ & VALID_FLAG);
}

inline string_view url::get_part_view(PartType t) const {
    if (t == SCHEME)
        return { norm_url_.data(), part_end_[SCHEME] };
    // begin & end offsets
    const std::size_t b = part_end_[t - 1] + detail::kPartStart[t];
    const std::size_t e = part_end_[t];
    return { norm_url_.data() + b, e > b ? e - b : 0 };
}

inline bool url::is_empty(const PartType t) const {
    if (t == SCHEME)
        return part_end_[SCHEME] == 0;
    // begin & end offsets
    const std::size_t b = part_end_[t - 1] + detail::kPartStart[t];
    const std::size_t e = part_end_[t];
    return b >= e;
}

inline bool url::is_null(const PartType t) const noexcept {
    return !(flags_ & (1u << t));
}

inline bool url::is_special_scheme() const noexcept {
    return scheme_inf_ && scheme_inf_->is_special;
}

inline bool url::is_file_scheme() const noexcept {
    return scheme_inf_ && scheme_inf_->is_file;
}

inline bool url::is_http_scheme() const noexcept {
    return scheme_inf_ && scheme_inf_->is_http;
}

inline bool url::has_credentials() const {
    return !is_empty(USERNAME) || !is_empty(PASSWORD);
}

// set scheme

template <class StringT>
inline void url::set_scheme_str(const StringT str) {
    norm_url_.clear(); // clear all
    part_end_[SCHEME] = str.length();
    norm_url_.append(str.data(), str.length());
    norm_url_ += ':';
}

inline void url::set_scheme(const url& src) {
    set_scheme_str(src.get_part_view(SCHEME));
    scheme_inf_ = src.scheme_inf_;
}

inline void url::set_scheme(const string_view str) {
    set_scheme_str(str);
    scheme_inf_ = get_scheme_info(str);
}

inline void url::set_scheme(std::size_t scheme_length) {
    part_end_[SCHEME] = scheme_length;
    scheme_inf_ = get_scheme_info(get_part_view(SCHEME));
}

// flags

inline void url::set_flag(const UrlFlag flag) noexcept {
    flags_ |= flag;
}

inline bool url::has_opaque_path() const noexcept {
    return !!(flags_ & OPAQUE_PATH_FLAG);
}

inline void url::set_has_opaque_path() noexcept {
    set_flag(OPAQUE_PATH_FLAG);
}

inline void url::set_host_type(const HostType ht) noexcept {
    flags_ = (flags_ & ~HOST_TYPE_MASK) | HOST_FLAG | (static_cast<unsigned int>(ht) << HOST_TYPE_SHIFT);
}

inline bool url::canHaveUsernamePasswordPort() const {
    return is_valid() && !(is_empty(url::HOST) || is_file_scheme());
}

// Private parsing constructor

template <class T, enable_if_str_arg_t<T>>
inline url::url(T&& str_url, const url* base, const char* what_arg) {
    const auto inp = make_str_arg(std::forward<T>(str_url));
    const auto res = do_parse(inp.begin(), inp.end(), base);
    if (res != validation_errc::ok)
        throw url_error(res, what_arg);
}

// Operations

inline void url::clear() {
    norm_url_.clear();
    part_end_.fill(0);
    scheme_inf_ = nullptr;
    flags_ = INITIAL_FLAGS;
    path_segment_count_ = 0;
    clear_search_params();
}

inline void url::swap(url& other) UPA_NOEXCEPT_17 {
    url tmp{ std::move(*this) };
    *this = std::move(other);
    other = std::move(tmp);
}

// Parser

// Implements "basic URL parser" https://url.spec.whatwg.org/#concept-basic-url-parser
// without encoding, url and state override arguments. It resets this url object to
// an empty value and then parses the input and modifies this url object.
// Returns validation_errc::ok on success, or an error value on parsing failure.
template <typename CharT>
inline validation_errc url::do_parse(const CharT* first, const CharT* last, const url* base) {
    const validation_errc res = [&]() {
        detail::url_serializer urls(*this);

        // reset URL
        urls.new_url();

        // is base URL valid?
        if (base && !base->is_valid())
            return validation_errc::invalid_base;

        // remove any leading and trailing C0 control or space:
        detail::do_trim(first, last);
        //TODO-WARN: validation error if trimmed

        return detail::url_parser::url_parse(urls, first, last, base);
    }();
    if (res == validation_errc::ok) {
        set_flag(VALID_FLAG);
        parse_search_params();
    }
    return res;
}

template <class T, enable_if_str_arg_t<T>>
validation_errc url::for_can_parse(T&& str_url, const url* base) {
    const auto inp = make_str_arg(std::forward<T>(str_url));
    const auto* first = inp.begin();
    const auto* last = inp.end();
    const validation_errc res = [&]() {
        detail::url_serializer urls(*this, false);

        // reset URL
        urls.new_url();

        // is base URL valid?
        if (base && !base->is_valid())
            return validation_errc::invalid_base;

        // remove any leading and trailing C0 control or space:
        detail::do_trim(first, last);
        //TODO-WARN: validation error if trimmed

        return detail::url_parser::url_parse(urls, first, last, base);
    }();
    if (res == validation_errc::ok)
        set_flag(VALID_FLAG);
    return res;
}

// Setters

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::href(StrT&& str) {
    url u; // parsedURL

    const auto inp = make_str_arg(std::forward<StrT>(str));
    if (u.do_parse(inp.begin(), inp.end(), nullptr) == validation_errc::ok) {
        safe_assign(std::move(u));
        return true;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::protocol(StrT&& str) {
    if (is_valid()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        return detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, detail::url_parser::scheme_start_state) == validation_errc::ok;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::username(StrT&& str) {
    if (canHaveUsernamePasswordPort()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));

        std::string& str_username = urls.start_part(url::USERNAME);
        // UTF-8 percent encode it using the userinfo encode set
        detail::append_utf8_percent_encoded(inp.begin(), inp.end(), userinfo_no_encode_set, str_username);
        urls.save_part();
        return true;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::password(StrT&& str) {
    if (canHaveUsernamePasswordPort()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));

        std::string& str_password = urls.start_part(url::PASSWORD);
        // UTF-8 percent encode it using the userinfo encode set
        detail::append_utf8_percent_encoded(inp.begin(), inp.end(), userinfo_no_encode_set, str_password);
        urls.save_part();
        return true;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::host(StrT&& str) {
    if (!has_opaque_path() && is_valid()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        return detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, detail::url_parser::host_state) == validation_errc::ok;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::hostname(StrT&& str) {
    if (!has_opaque_path() && is_valid()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        return detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, detail::url_parser::hostname_state) == validation_errc::ok;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::port(StrT&& str) {
    if (canHaveUsernamePasswordPort()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        const auto* first = inp.begin();
        const auto* last = inp.end();

        if (first == last) {
            urls.clear_part(url::PORT);
            return true;
        }
        return detail::url_parser::url_parse(urls, first, last, nullptr, detail::url_parser::port_state) == validation_errc::ok;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::pathname(StrT&& str) {
    if (!has_opaque_path() && is_valid()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        return detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, detail::url_parser::path_start_state) == validation_errc::ok;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::search(StrT&& str) {
    bool res = false;
    if (is_valid()) {
        {
            detail::url_setter urls(*this);

            const auto inp = make_str_arg(std::forward<StrT>(str));
            const auto* first = inp.begin();
            const auto* last = inp.end();

            if (first == last) {
                urls.clear_part(url::QUERY);
                // empty context object's query object's list
                clear_search_params();
                urls.potentially_strip_trailing_spaces_from_an_opaque_path();
                return true;
            }
            if (*first == '?') ++first;
            res = detail::url_parser::url_parse(urls, first, last, nullptr, detail::url_parser::query_state) == validation_errc::ok;
        }
        // set context object's query object's list to the result of parsing input
        parse_search_params();
    }
    return res;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::hash(StrT&& str) {
    if (is_valid()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        const auto* first = inp.begin();
        const auto* last = inp.end();

        if (first == last) {
            urls.clear_part(url::FRAGMENT);
            urls.potentially_strip_trailing_spaces_from_an_opaque_path();
            return true;
        }
        if (*first == '#') ++first;
        return detail::url_parser::url_parse(urls, first, last, nullptr, detail::url_parser::fragment_state) == validation_errc::ok;
    }
    return false;
}


namespace detail {

// Implements "basic URL parser" https://url.spec.whatwg.org/#concept-basic-url-parser
// without 1 step. It modifies the URL stored in the urls object.
// Returns validation_errc::ok on success, or an error value on parsing failure.
template <typename CharT>
inline validation_errc url_parser::url_parse(url_serializer& urls, const CharT* first, const CharT* last, const url* base, State state_override)
{
    using UCharT = typename std::make_unsigned<CharT>::type;

    // remove all ASCII tab or newline from URL
    simple_buffer<CharT> buff_no_ws;
    detail::do_remove_whitespace(first, last, buff_no_ws);
    //TODO-WARN: validation error if removed

    if (urls.need_save()) {
        // reserve size (TODO: But what if `base` is used?)
        const auto length = std::distance(first, last);
        urls.reserve(length + 32);
    }

#ifdef UPA_URL_USE_ENCODING
    const char* encoding = "UTF-8";
    // TODO: If encoding override is given, set encoding to the result of getting an output encoding from encoding override.
#endif

    auto pointer = first;
    State state = state_override ? state_override : scheme_start_state;

    // has scheme?
    if (state == scheme_start_state) {
        if (pointer != last && detail::is_first_scheme_char(*pointer)) {
            state = scheme_state; // this appends first char to buffer
        } else if (!state_override) {
            state = no_scheme_state;
        } else {
            // 3. Otherwise, return failure.
            return validation_errc::scheme_invalid_code_point;
        }
    }

    if (state == scheme_state) {
        // Deviation from URL stdandart's [ 2. ... if c is ":", run ... ] to
        // [ 2. ... if c is ":", or EOF and state override is given, run ... ]
        // This lets protocol setter to pass input without adding ':' to the end.
        // Similiar deviation exists in nodejs, see:
        // https://github.com/nodejs/node/pull/11917#pullrequestreview-28061847

        // first scheme char has been checked in the scheme_start_state, so skip it
        const auto end_of_scheme = std::find_if_not(pointer + 1, last, detail::is_scheme_char<CharT>);
        const bool is_scheme = end_of_scheme != last
            ? *end_of_scheme == ':'
            : state_override != not_set_state;

        if (is_scheme) {
            // start of scheme
            std::string& str_scheme = urls.start_scheme();
            // Append scheme chars: it is safe to set the 0x20 bit on all code points -
            // it lowercases ASCII alphas, while other code points allowed in a scheme
            // (0 - 9, +, -, .) already have this bit set.
            for (auto it = pointer; it != end_of_scheme; ++it)
                str_scheme.push_back(static_cast<char>(*it | 0x20));

            if (state_override) {
                const url::scheme_info* scheme_inf = url::get_scheme_info(str_scheme);
                const bool is_special_old = urls.is_special_scheme();
                const bool is_special_new = scheme_inf && scheme_inf->is_special;
                if (is_special_old != is_special_new)
                    return validation_errc::ignored;
                // new URL("http://u:p@host:88/).protocol("file:");
                if (scheme_inf && scheme_inf->is_file && (urls.has_credentials() || !urls.is_null(url::PORT)))
                    return validation_errc::ignored;
                // new URL("file:///path).protocol("http:");
                if (urls.is_file_scheme() && urls.is_empty(url::HOST))
                    return validation_errc::ignored;
                // OR ursl.is_empty(url::HOST) && scheme_inf->no_empty_host

                // set url's scheme
                urls.save_scheme();

                // https://github.com/whatwg/url/pull/328
                // optimization: compare ports if scheme has the default port
                if (scheme_inf && scheme_inf->default_port >= 0 &&
                    urls.port_int() == scheme_inf->default_port) {
                    // set url's port to null
                    urls.clear_part(url::PORT);
                }

                // if state override is given, then return
                return validation_errc::ok;
            }
            urls.save_scheme();

            pointer = end_of_scheme + 1; // skip ':'
            if (urls.is_file_scheme()) {
                // TODO-WARN: if remaining does not start with "//", validation error.
                state = file_state;
            } else {
                if (urls.is_special_scheme()) {
                    if (base && urls.get_part_view(url::SCHEME) == base->get_part_view(url::SCHEME)) {
                        assert(base->is_special_scheme()); // and therefore does not have an opaque path
                        state = special_relative_or_authority_state;
                    } else {
                        state = special_authority_slashes_state;
                    }
                } else if (pointer < last && *pointer == '/') {
                    state = path_or_authority_state;
                    ++pointer;
                } else {
                    // set url’s path to the empty string (so path becomes opaque,
                    // see: https://url.spec.whatwg.org/#url-opaque-path)
                    urls.set_has_opaque_path();
                    // To complete the set url's path to the empty string, following functions must be called:
                    //  urls.start_path_string();
                    //  urls.save_path_string();
                    // but the same functions will be called in the opaque_path_state, so skip them here.
                    state = opaque_path_state;
                }
            }
        } else if (!state_override) {
            state = no_scheme_state;
        } else {
            // 4. Otherwise, return failure.
            return validation_errc::scheme_invalid_code_point;
        }
    }

    if (state == no_scheme_state) {
        if (base) {
            if (base->has_opaque_path()) {
                if (pointer < last && *pointer == '#') {
                    urls.set_scheme(*base);
                    urls.append_parts(*base, url::PATH, url::QUERY);
                    //TODO: url's fragment to the empty string
                    state = fragment_state;
                    ++pointer;
                } else {
                    // 1. If ..., or base has an opaque path and c is not U+0023 (#),
                    // missing-scheme-non-relative-URL validation error, return failure.
                    return validation_errc::missing_scheme_non_relative_url;
                }
            } else {
                state = base->is_file_scheme() ? file_state : relative_state;
            }
        } else {
            // 1. If base is null, ..., missing-scheme-non-relative-URL
            // validation error, return failure
            return validation_errc::missing_scheme_non_relative_url;
        }
    }

    if (state == special_relative_or_authority_state) {
        if (last - pointer > 1 && pointer[0] == '/' && pointer[1] == '/') {
            state = special_authority_ignore_slashes_state;
            pointer += 2; // skip "//"
        } else {
            //TODO-WARN: validation error
            state = relative_state;
        }
    }

    if (state == path_or_authority_state) {
        if (pointer < last && pointer[0] == '/') {
            state = authority_state;
            ++pointer; // skip "/"
        } else {
            state = path_state;
        }
    }

    if (state == relative_state) {
        // std::assert(base != nullptr);
        urls.set_scheme(*base);
        if (pointer == last) {
            // EOF code point
            // Set url's username to base's username, url's password to base's password, url's host to base's host,
            // url's port to base's port, url's path to base's path, and url's query to base's query
            urls.append_parts(*base, url::USERNAME, url::QUERY);
            return validation_errc::ok; // EOF
        }
        const CharT ch = *pointer++;
        switch (ch) {
        case '/':
            state = relative_slash_state;
            break;
        case '?':
            // Set url's username to base's username, url's password to base's password, url's host to base's host,
            // url's port to base's port, url's path to base's path, url's query to the empty string, and state to query state.
            urls.append_parts(*base, url::USERNAME, url::PATH);
            state = query_state;    // sets query to the empty string
            break;
        case '#':
            // Set url's username to base's username, url's password to base's password, url's host to base's host,
            // url's port to base's port, url's path to base's path, url's query to base's query, url's fragment to the empty string
            urls.append_parts(*base, url::USERNAME, url::QUERY);
            state = fragment_state; // sets fragment to the empty string
            break;
        case '\\':
            if (urls.is_special_scheme()) {
                //TODO-WARN: validation error
                state = relative_slash_state;
                break;
            }
            UPA_FALLTHROUGH
        default:
            // Set url's username to base's username, url's password to base's password, url's host to base's host,
            // url's port to base's port, url's path to base's path, and then remove url's path's last entry, if any
            urls.append_parts(*base, url::USERNAME, url::PATH, &url::get_path_rem_last);
            state = path_state;
            --pointer;
        }
    }

    if (state == relative_slash_state) {
        // EOF ==> 0 ==> default:
        switch (pointer != last ? *pointer : 0) {
        case '/':
            if (urls.is_special_scheme())
                state = special_authority_ignore_slashes_state;
            else
                state = authority_state;
            ++pointer;
            break;
        case '\\':
            if (urls.is_special_scheme()) {
                // TODO-WARN: validation error
                state = special_authority_ignore_slashes_state;
                ++pointer;
                break;
            }
            UPA_FALLTHROUGH
        default:
            // set url's username to base's username, url's password to base's password, url's host to base's host,
            // url's port to base's port
            urls.append_parts(*base, url::USERNAME, url::PORT);
            state = path_state;
        }
    }

    if (state == special_authority_slashes_state) {
        if (last - pointer > 1 && pointer[0] == '/' && pointer[1] == '/') {
            state = special_authority_ignore_slashes_state;
            pointer += 2; // skip "//"
        } else {
            //TODO-WARN: validation error
            state = special_authority_ignore_slashes_state;
        }
    }

    if (state == special_authority_ignore_slashes_state) {
        auto it = pointer;
        while (it < last && detail::is_slash(*it)) ++it;
        // if (it != pointer) // TODO-WARN: validation error
        pointer = it;
        state = authority_state;
    }

    // TODO?: credentials serialization do after host parsing, because
    // if host is null, then no credentials serialization
    if (state == authority_state) {
        // TODO: saugoti end_of_authority ir naudoti kituose state
        const auto end_of_authority = urls.is_special_scheme() ?
            std::find_if(pointer, last, detail::is_special_authority_end_char<CharT>) :
            std::find_if(pointer, last, detail::is_authority_end_char<CharT>);

        const auto it_eta = detail::find_last(pointer, end_of_authority, static_cast<CharT>('@'));
        if (it_eta != end_of_authority) {
            if (std::distance(it_eta, end_of_authority) == 1) {
                // 2.1. If atSignSeen is true and buffer is the empty string, host-missing
                // validation error, return failure.
                // Example: "http://u:p@/"
                return validation_errc::host_missing;
            }
            //TODO-WARN: validation error
            if (urls.need_save()) {
                const auto it_colon = std::find(pointer, it_eta, ':');
                // url includes credentials?
                const bool not_empty_password = std::distance(it_colon, it_eta) > 1;
                if (not_empty_password || std::distance(pointer, it_colon) > 0 /*not empty username*/) {
                    // username
                    std::string& str_username = urls.start_part(url::USERNAME);
                    detail::append_utf8_percent_encoded(pointer, it_colon, userinfo_no_encode_set, str_username); // UTF-8 percent encode, @ -> %40
                    urls.save_part();
                    // password
                    if (not_empty_password) {
                        std::string& str_password = urls.start_part(url::PASSWORD);
                        detail::append_utf8_percent_encoded(it_colon + 1, it_eta, userinfo_no_encode_set, str_password); // UTF-8 percent encode, @ -> %40
                        urls.save_part();
                    }
                }
            }
            // after '@'
            pointer = it_eta + 1;
        }
        state = host_state;
    }

    if (state == host_state || state == hostname_state) {
        if (state_override && urls.is_file_scheme()) {
            state = file_host_state;
        } else {
            const auto end_of_authority = urls.is_special_scheme() ?
                std::find_if(pointer, last, detail::is_special_authority_end_char<CharT>) :
                std::find_if(pointer, last, detail::is_authority_end_char<CharT>);

            bool in_square_brackets = false; // [] flag
            bool is_port = false;
            auto it_host_end = pointer;
            for (; it_host_end < end_of_authority; ++it_host_end) {
                const CharT ch = *it_host_end;
                if (ch == ':') {
                    if (!in_square_brackets) {
                        is_port = true;
                        break;
                    }
                } else if (ch == '[') {
                    in_square_brackets = true;
                } else if (ch == ']') {
                    in_square_brackets = false;
                }
            }

            // if buffer is the empty string
            if (pointer == it_host_end) {
                // make sure that if port is present or scheme is special, host is non-empty
                if (is_port || urls.is_special_scheme()) {
                    // host-missing validation error, return failure
                    return validation_errc::host_missing;
                }
                // 3.2. if state override is given, buffer is the empty string, and either
                // url includes credentials or url’s port is non-null, return.
                if (state_override && (urls.has_credentials() || !urls.is_null(url::PORT))) {
                    return validation_errc::ignored; // can not make host empty
                }
            }

            // 2.2. If state override is given and state override is hostname state, then return
            if (is_port && state_override == hostname_state)
                return validation_errc::ignored; // host with port not accepted

            // parse and set host:
            const auto res = parse_host(urls, pointer, it_host_end);
            // 2.4, 3.4. If host is failure, then return failure.
            if (res != validation_errc::ok)
                return res;

            if (is_port) {
                pointer = it_host_end + 1; // skip ':'
                state = port_state;
            } else {
                pointer = it_host_end;
                state = path_start_state;
                if (state_override)
                    return validation_errc::ok;
            }
        }
    }

    if (state == port_state) {
        const auto end_of_digits = std::find_if_not(pointer, last, detail::is_ascii_digit<CharT>);

        const bool is_end_of_authority =
            end_of_digits == last || // EOF
            detail::is_authority_end_char(end_of_digits[0]) ||
            (end_of_digits[0] == '\\' && urls.is_special_scheme());

        if (is_end_of_authority || state_override) {
            if (pointer < end_of_digits) {
                // url string contains port
                // skip the leading zeros except the last
                pointer = std::find_if(pointer, end_of_digits - 1, [](CharT c) { return c != '0'; });
                // check port <= 65535 (0xFFFF)
                if (std::distance(pointer, end_of_digits) > 5)
                    return validation_errc::port_out_of_range;
                // port length <= 5
                int port = 0;
                for (auto it = pointer; it < end_of_digits; ++it)
                    port = port * 10 + (*it - '0');
                // 2.1.2. If port is greater than 2^16 − 1, port-out-of-range
                // validation error, return failure
                if (port > 0xFFFF)
                    return validation_errc::port_out_of_range;
                if (urls.need_save()) {
                    // set port if not default
                    if (urls.scheme_inf() == nullptr || urls.scheme_inf()->default_port != port) {
                        util::append(urls.start_part(url::PORT), str_arg<CharT>{ pointer, end_of_digits });
                        urls.save_part();
                        urls.set_flag(url::PORT_FLAG);
                    } else {
                        // (2-1-3) Set url's port to null
                        urls.clear_part(url::PORT);
                    }
                }
                // 2.2. If state override is given, then return
                if (state_override)
                    return validation_errc::ok;
            } else if (state_override)
                return validation_errc::ignored;
            state = path_start_state;
            pointer = end_of_digits;
        } else {
            // 3. Otherwise, port-invalid validation error, return failure (contains non-digit)
            return validation_errc::port_invalid;
        }
    }

    if (state == file_state) {
        if (!urls.is_file_scheme())
            urls.set_scheme(string_view{ "file", 4 });
        // ensure file URL's host is not null
        urls.set_empty_host();
        // EOF ==> 0 ==> default:
        switch (pointer != last ? *pointer : 0) {
        case '\\':
            // TODO-WARN: validation error
        case '/':
            state = file_slash_state;
            ++pointer;
            break;

        default:
            if (base && base->is_file_scheme()) {
                if (pointer == last) {
                    // EOF code point
                    // Set url's host to base's host, url's path to base's path, and url's query to base's query
                    urls.append_parts(*base, url::HOST, url::QUERY);
                    return validation_errc::ok; // EOF
                }
                switch (*pointer) {
                case '?':
                    // Set url's host to base's host, url's path to base's path, url's query to the empty string
                    urls.append_parts(*base, url::HOST, url::PATH);
                    state = query_state; // sets query to the empty string
                    ++pointer;
                    break;
                case '#':
                    // Set url's host to base's host, url's path to base's path, url's query to base's query, url's fragment to the empty string
                    urls.append_parts(*base, url::HOST, url::QUERY);
                    state = fragment_state; // sets fragment to the empty string
                    ++pointer;
                    break;
                default:
                    if (!detail::starts_with_windows_drive(pointer, last)) {
                        // set url's host to base's host, url's path to base's path, and then shorten url's path
                        urls.append_parts(*base, url::HOST, url::PATH, &url::get_shorten_path);
                        // Note: This is a (platform-independent) Windows drive letter quirk.
                    } else {
                        // TODO-WARN: validation error
                        // set url's host to base's host
                        urls.append_parts(*base, url::HOST, url::HOST);
                    }
                    state = path_state;
                }
            } else {
                state = path_state;
            }
        }
    }

    if (state == file_slash_state) {
        // EOF ==> 0 ==> default:
        switch (pointer != last ? *pointer : 0) {
        case '\\':
            // TODO-WARN: validation error
        case '/':
            state = file_host_state;
            ++pointer;
            break;

        default:
            if (base && base->is_file_scheme() && urls.need_save()) {
                // It is important to first set host, then path, otherwise serializer
                // will fail.

                // set url's host to base's host
                urls.append_parts(*base, url::HOST, url::HOST);
                // path
                if (!detail::starts_with_windows_drive(pointer, last)) {
                    const string_view base_path = base->get_path_first_string(2);
                    // if base's path[0] is a normalized Windows drive letter
                    if (base_path.length() == 2 &&
                        detail::is_normalized_windows_drive(base_path[0], base_path[1])) {
                        // append base's path[0] to url's path
                        std::string& str_path = urls.start_path_segment();
                        str_path.append(base_path.data(), 2); // "C:"
                        urls.save_path_segment();
                        // Note: This is a (platform - independent) Windows drive letter quirk.
                    }
                }
            }
            state = path_state;
        }
    }

    if (state == file_host_state) {
        const auto end_of_authority = std::find_if(pointer, last, detail::is_special_authority_end_char<CharT>);

        if (pointer == end_of_authority) {
            // buffer is the empty string
            // set empty host
            urls.set_empty_host();
            // if state override is given, then return
            if (state_override)
                return validation_errc::ok;
            state = path_start_state;
        } else if (!state_override && end_of_authority - pointer == 2 &&
            detail::is_windows_drive(pointer[0], pointer[1])) {
            // buffer is a Windows drive letter
            // TODO-WARN: validation error
            state = path_state;
            // Note: This is a (platform - independent) Windows drive letter quirk.
            // buffer is not reset here and instead used in the path state.
            // TODO: buffer is not reset here and instead used in the path state
        } else {
            // parse and set host:
            const auto res = parse_host(urls, pointer, end_of_authority);
            if (res != validation_errc::ok || !urls.need_save())
                return res; // TODO-ERR: failure
            // if host is "localhost", then set host to the empty string
            if (urls.get_part_view(url::HOST) == string_view{ "localhost", 9 }) {
                // set empty host
                urls.empty_host();
            }
            // if state override is given, then return
            if (state_override)
                return validation_errc::ok;
            pointer = end_of_authority;
            state = path_start_state;
        }
    }

    if (!urls.need_save())
        return validation_errc::ok;

    if (state == path_start_state) {
        if (urls.is_special_scheme()) {
            if (pointer != last) {
                switch (*pointer) {
                case '\\':
                    // TODO-WARN: validation error
                case '/':
                    ++pointer;
                }
            }
            state = path_state;
        } else if (pointer != last) {
            if (!state_override) {
                switch (pointer[0]) {
                case '?':
                    // TODO: set url's query to the empty string
                    state = query_state;
                    ++pointer;
                    break;
                case '#':
                    // TODO: set url's fragment to the empty string
                    state = fragment_state;
                    ++pointer;
                    break;
                case '/':
                    ++pointer;
                    UPA_FALLTHROUGH
                default:
                    state = path_state;
                    break;
                }
            } else {
                if (pointer[0] == '/') ++pointer;
                state = path_state;
            }
        } else {
            // EOF
            if (state_override && urls.is_null(url::HOST))
                urls.append_empty_path_segment();
            // otherwise path is empty
            urls.commit_path();
            return validation_errc::ok;
        }
    }

    if (state == path_state) {
        const auto end_of_path = state_override ? last :
            std::find_if(pointer, last, [](CharT c) { return c == '?' || c == '#'; });

        parse_path(urls, pointer, end_of_path);
        pointer = end_of_path;

        // the end of path parse
        urls.commit_path();

        if (pointer == last)
            return validation_errc::ok; // EOF

        const CharT ch = *pointer++;
        if (ch == '?') {
            // TODO: set url's query to the empty string
            state = query_state;
        } else {
            // ch == '#'
            // TODO: set url's fragment to the empty string
            state = fragment_state;
        }
    }

    if (state == opaque_path_state) {
        const auto end_of_path =
            std::find_if(pointer, last, [](CharT c) { return c == '?' || c == '#'; });

        // UTF-8 percent encode using the C0 control percent-encode set,
        // and append the result to url's path string
        std::string& str_path = urls.start_path_string();
        do_simple_path(pointer, end_of_path, str_path);
        urls.save_path_string();
        pointer = end_of_path;

        if (pointer == last)
            return validation_errc::ok; // EOF

        const CharT ch = *pointer++;
        if (ch == '?') {
            // TODO: set url's query to the empty string
            state = query_state;
        } else {
            // ch == '#'
            // TODO: set url's fragment to the empty string
            state = fragment_state;
        }
    }

    if (state == query_state) {
        const auto end_of_query = state_override ? last : std::find(pointer, last, '#');

        // TODO-WARN:
        //for (auto it = pointer; it < end_of_query; ++it) {
        //  UCharT c = static_cast<UCharT>(*it);
        //  // 1. If c is not a URL code point and not "%", validation error.
        //  // 2. If c is "%" and remaining does not start with two ASCII hex digits, validation error.
        //}

#ifdef UPA_URL_USE_ENCODING
        // scheme_inf_ == nullptr, if unknown scheme
        if (!urls.scheme_inf() || !urls.scheme_inf()->is_special || urls.scheme_inf()->is_ws)
            encoding = "UTF-8";
#endif

        // Let query_cpset be the special-query percent-encode set if url is special;
        // otherwise the query percent-encode set.
        const auto& query_cpset = urls.is_special_scheme()
            ? special_query_no_encode_set
            : query_no_encode_set;

        // Percent-encode after encoding, with encoding, buffer, and query_cpset, and append
        // the result to url’s query.
        // TODO: now supports UTF-8 encoding only, maybe later add other encodings
        std::string& str_query = urls.start_part(url::QUERY);
        // detail::append_utf8_percent_encoded(pointer, end_of_query, query_cpset, str_query);
        while (pointer != end_of_query) {
            // UTF-8 percent encode c using the fragment percent-encode set
            // and ignore '\0'
            const auto uch = static_cast<UCharT>(*pointer);
            if (uch >= 0x80) {
                // invalid utf-8/16/32 sequences will be replaced with kUnicodeReplacementCharacter
                detail::append_utf8_percent_encoded_char(pointer, end_of_query, str_query);
            } else {
                // Just append the 7-bit character, possibly percent encoding it
                const auto uc = static_cast<unsigned char>(uch);
                if (!detail::is_char_in_set(uc, query_cpset))
                    detail::append_percent_encoded_byte(uc, str_query);
                else
                    str_query.push_back(uc);
                ++pointer;
            }
            // TODO-WARN:
            // If c is not a URL code point and not "%", validation error.
            // If c is "%" and remaining does not start with two ASCII hex digits, validation error.
            // Let bytes be the result of encoding c using encoding ...
        }
        urls.save_part();
        urls.set_flag(url::QUERY_FLAG);

        pointer = end_of_query;
        if (pointer == last)
            return validation_errc::ok; // EOF
        // *pointer == '#'
        //TODO: set url's fragment to the empty string
        state = fragment_state;
        ++pointer; // skip '#'
    }

    if (state == fragment_state) {
        // https://url.spec.whatwg.org/#fragment-state
        std::string& str_frag = urls.start_part(url::FRAGMENT);
        while (pointer < last) {
            // UTF-8 percent encode c using the fragment percent-encode set
            const auto uch = static_cast<UCharT>(*pointer);
            if (uch >= 0x80) {
                // invalid utf-8/16/32 sequences will be replaced with kUnicodeReplacementCharacter
                detail::append_utf8_percent_encoded_char(pointer, last, str_frag);
            } else {
                // Just append the 7-bit character, possibly percent encoding it
                const auto uc = static_cast<unsigned char>(uch);
                if (detail::is_char_in_set(uc, fragment_no_encode_set)) {
                    str_frag.push_back(uc);
                } else {
                    // other characters are percent encoded
                    detail::append_percent_encoded_byte(uc, str_frag);
                }
                ++pointer;
            }
            // TODO-WARN:
            // If c is not a URL code point and not "%", validation error.
            // If c is "%" and remaining does not start with two ASCII hex digits, validation error.
        }
        urls.save_part();
        urls.set_flag(url::FRAGMENT_FLAG);
    }

    return validation_errc::ok;
}

// internal functions

template <typename CharT>
inline validation_errc url_parser::parse_host(url_serializer& urls, const CharT* first, const CharT* last) {
    return host_parser::parse_host(first, last, !urls.is_special_scheme(), urls);
}

template <typename CharT>
inline void url_parser::parse_path(url_serializer& urls, const CharT* first, const CharT* last) {
    // path state; includes:
    // 1. [ (/,\) - 1, 2, 3, 4 - [ 1 (if first segment), 2 ] ]
    // 2. [ 1 ... 4 ]
    static const auto escaped_dot = [](const CharT* const pointer) -> bool {
        // "%2e" or "%2E"
        return pointer[0] == '%' && pointer[1] == '2' && (pointer[2] | 0x20) == 'e';
    };
    static const auto double_dot = [](const CharT* const pointer, const std::size_t len) -> bool {
        switch (len) {
        case 2: // ".."
            return pointer[0] == '.' && pointer[1] == '.';
        case 4: // ".%2e" or "%2e."
            return (pointer[0] == '.' && escaped_dot(pointer + 1)) ||
                (escaped_dot(pointer) && pointer[3] == '.');
        case 6: // "%2e%2e"
            return escaped_dot(pointer) && escaped_dot(pointer + 3);
        default:
            return false;
        }
    };
    static const auto single_dot = [](const CharT* const pointer, const std::size_t len) -> bool {
        switch (len) {
        case 1: return pointer[0] == '.';
        case 3: return escaped_dot(pointer); // "%2e"
        default: return false;
        }
    };

    // parse path's segments
    auto pointer = first;
    while (true) {
        const auto end_of_segment = urls.is_special_scheme()
            ? std::find_if(pointer, last, detail::is_slash<CharT>)
            : std::find(pointer, last, '/');

        // end_of_segment >= pointer
        const std::size_t len = end_of_segment - pointer;
        const bool is_last = end_of_segment == last;
        // TODO-WARN: 1. If url is special and c is "\", validation error.

        if (double_dot(pointer, len)) {
            urls.shorten_path();
            if (is_last) urls.append_empty_path_segment();
        } else if (single_dot(pointer, len)) {
            if (is_last) urls.append_empty_path_segment();
        } else {
            if (len == 2 &&
                urls.is_file_scheme() &&
                urls.is_empty_path() &&
                detail::is_windows_drive(pointer[0], pointer[1]))
            {
                // replace the second code point in buffer with ":"
                std::string& str_path = urls.start_path_segment();
                str_path += static_cast<char>(pointer[0]);
                str_path += ':';
                urls.save_path_segment();
                //Note: This is a (platform-independent) Windows drive letter quirk.
            } else {
                std::string& str_path = urls.start_path_segment();
                do_path_segment(pointer, end_of_segment, str_path);
                urls.save_path_segment();
                // end of segment
                pointer = end_of_segment;
            }
        }
        // next segment
        if (is_last) break;
        pointer = end_of_segment + 1; // skip '/' or '\'
    }
}

template <typename CharT>
inline bool url_parser::do_path_segment(const CharT* pointer, const CharT* last, std::string& output) {
    using UCharT = typename std::make_unsigned<CharT>::type;

    // TODO-WARN: 2. [ 1 ... 2 ] validation error.
    bool success = true;
    while (pointer < last) {
        // UTF-8 percent encode c using the default encode set
        const auto uch = static_cast<UCharT>(*pointer);
        if (uch >= 0x80) {
            // invalid utf-8/16/32 sequences will be replaced with 0xfffd
            success &= detail::append_utf8_percent_encoded_char(pointer, last, output);
        } else {
            // Just append the 7-bit character, possibly percent encoding it
            const auto uc = static_cast<unsigned char>(uch);
            if (!detail::is_char_in_set(uc, path_no_encode_set))
                detail::append_percent_encoded_byte(uc, output);
            else
                output.push_back(uc);
            ++pointer;
        }
    }
    return success;
}

template <typename CharT>
inline bool url_parser::do_simple_path(const CharT* pointer, const CharT* last, std::string& output) {
    using UCharT = typename std::make_unsigned<CharT>::type;

    // 3. of "opaque path state"
    // TODO-WARN: 3. [ 1 ... 2 ] validation error.
    //  1. If c is not EOF code point, not a URL code point, and not "%", validation error.
    //  2. If c is "%" and remaining does not start with two ASCII hex digits, validation error.

    bool success = true;
    while (pointer < last) {
        // UTF-8 percent encode c using the C0 control percent-encode set (U+0000 ... U+001F and >U+007E)
        const auto uch = static_cast<UCharT>(*pointer);
        if (uch >= 0x7f) {
            // invalid utf-8/16/32 sequences will be replaced with 0xfffd
            success &= detail::append_utf8_percent_encoded_char(pointer, last, output);
        } else {
            // Just append the 7-bit character, percent encoding C0 control chars
            const auto uc = static_cast<unsigned char>(uch);
            if (uc <= 0x1f)
                detail::append_percent_encoded_byte(uc, output);
            else
                output.push_back(uc);
            ++pointer;
        }
    }
    return success;
}

} // namespace detail


// path util

inline string_view url::get_path_first_string(std::size_t len) const {
    string_view pathv = get_part_view(PATH);
    if (pathv.length() == 0 || has_opaque_path())
        return pathv;
    // skip '/'
    pathv.remove_prefix(1);
    if (pathv.length() == len || (pathv.length() > len && pathv[len] == '/')) {
        return { pathv.data(), len };
    }
    return { pathv.data(), 0 };
}

// path shortening

inline bool url::get_path_rem_last(std::size_t& path_end, std::size_t& path_segment_count) const {
    if (path_segment_count_ > 0) {
        // Remove path's last item
        const char* const first = norm_url_.data() + part_end_[url::PATH-1];
        const char* const last = norm_url_.data() + part_end_[url::PATH];
        const char* it = detail::find_last(first, last, '/');
        if (it == last) it = first; // remove full path if '/' not found
        // shorten
        path_end = it - norm_url_.data();
        path_segment_count = path_segment_count_ - 1;
        return true;
    }
    return false;
}

// https://url.spec.whatwg.org/#shorten-a-urls-path

inline bool url::get_shorten_path(std::size_t& path_end, std::size_t& path_segment_count) const {
    assert(!has_opaque_path());
    if (path_segment_count_ == 0)
        return false;
    if (is_file_scheme() && path_segment_count_ == 1) {
        const string_view path1 = get_path_first_string(2);
        if (path1.length() == 2 &&
            detail::is_normalized_windows_drive(path1[0], path1[1]))
            return false;
    }
    // Remove path's last item
    return get_path_rem_last(path_end, path_segment_count);
}


namespace detail {

// url_serializer class

inline void url_serializer::shorten_path() {
    assert(last_pt_ <= url::PATH);
    if (url_.get_shorten_path(url_.part_end_[url::PATH], url_.path_segment_count_))
        url_.norm_url_.resize(url_.part_end_[url::PATH]);
}

// set scheme

inline std::string& url_serializer::start_scheme() {
    url_.norm_url_.clear(); // clear all
    return url_.norm_url_;
}

inline void url_serializer::save_scheme() {
    set_scheme(url_.norm_url_.length());
    url_.norm_url_.push_back(':');
}

// set url's part

inline void url_serializer::fill_parts_offset(url::PartType t1, url::PartType t2, std::size_t offset) {
    for (int ind = t1; ind < t2; ++ind)
        url_.part_end_[ind] = offset;
}

inline std::string& url_serializer::start_part(url::PartType new_pt) {
    // offsets of empty parts (until new_pt) are also filled
    auto fill_start_pt = static_cast<url::PartType>(static_cast<int>(last_pt_)+1);
    switch (last_pt_) {
    case url::SCHEME:
        // if host is non-null
        if (new_pt <= url::HOST)
            url_.norm_url_.append("//");
        break;
    case url::USERNAME:
        if (new_pt == url::PASSWORD) {
            url_.norm_url_ += ':';
            break;
        } else {
            url_.part_end_[url::PASSWORD] = url_.norm_url_.length();
            fill_start_pt = url::HOST_START; // (url::PASSWORD + 1)
        }
        UPA_FALLTHROUGH
    case url::PASSWORD:
        if (new_pt == url::HOST)
            url_.norm_url_ += '@';
        break;
    case url::HOST:
    case url::PORT:
        break;
    case url::PATH:
        if (new_pt == url::PATH) // continue on path
            return url_.norm_url_;
        break;
    default: break;
    }

    fill_parts_offset(fill_start_pt, new_pt, url_.norm_url_.length());

    switch (new_pt) {
    case url::PORT:
        url_.norm_url_ += ':';
        break;
    case url::QUERY:
        url_.norm_url_ += '?';
        break;
    case url::FRAGMENT:
        url_.norm_url_ += '#';
        break;
    default: break;
    }

    assert(last_pt_ < new_pt || (last_pt_ == new_pt && is_empty(last_pt_)));
    // value to url_.part_end_[new_pt] will be assigned in the save_part()
    last_pt_ = new_pt;
    return url_.norm_url_;
}

inline void url_serializer::save_part() {
    url_.part_end_[last_pt_] = url_.norm_url_.length();
}

// The append_empty_path_segment() appends the empty string to url’s path (list);
// it is called from these places:
// 1) path_start_state -> [5.]
// 2) path_state -> [1.2.2. ".." ]
// 3) path_state -> [1.3. "." ]
inline void url_serializer::append_empty_path_segment() {
    start_path_segment();
    save_path_segment();
}

inline std::string& url_serializer::start_path_segment() {
    // appends new segment to path: / seg1 / seg2 / ... / segN
    std::string& str_path = start_part(url::PATH);
    str_path += '/';
    return str_path;
}

inline void url_serializer::save_path_segment() {
    save_part();
    url_.path_segment_count_++;
}

inline void url_serializer::commit_path() {
    // "/." path prefix
    adjust_path_prefix();
}

inline void url_serializer::adjust_path_prefix() {
    // "/." path prefix
    // https://url.spec.whatwg.org/#url-serializing (4.1.)
    string_view new_prefix;
    if (is_null(url::HOST) && url_.path_segment_count_ > 1) {
        const auto pathname = get_part_view(url::PATH);
        if (pathname.length() > 1 && pathname[0] == '/' && pathname[1] == '/')
            new_prefix = { "/.", 2 };
    }
    if (is_empty(url::PATH_PREFIX) != new_prefix.empty())
        replace_part(url::PATH_PREFIX, new_prefix.data(), new_prefix.length());
}

inline std::string& url_serializer::start_path_string() {
    return start_part(url::PATH);
}

inline void url_serializer::save_path_string() {
    assert(url_.path_segment_count_ == 0);
    save_part();
}


inline void url_serializer::set_empty_host() {
    start_part(url::HOST);
    save_part();
    set_host_type(HostType::Empty);
}

inline void url_serializer::empty_host() {
    // It is called right after a host parsing
    assert(last_pt_ == url::HOST);

    const std::size_t host_end = url_.part_end_[url::HOST_START];
    url_.part_end_[url::HOST] = host_end;
    url_.norm_url_.resize(host_end);

    url_.set_host_type(HostType::Empty);
}

// host_output overrides

inline std::string& url_serializer::hostStart() {
    return start_part(url::HOST);
}

inline void url_serializer::hostDone(HostType ht) {
    save_part();
    set_host_type(ht);

    // non-null host
    if (!is_empty(url::PATH_PREFIX)) {
        // remove '/.' path prefix
        replace_part(url::PATH_PREFIX, nullptr, 0);
    }
}

// append parts from other url

inline void url_serializer::append_parts(const url& src, url::PartType t1, url::PartType t2, PathOpFn pathOpFn) {
    // See URL serializing
    // https://url.spec.whatwg.org/#concept-url-serializer
    const url::PartType ifirst = [&]() {
        if (t1 <= url::HOST) {
            // authority, host
            if (!src.is_null(url::HOST)) {
                if (t1 == url::USERNAME && src.has_credentials())
                    return url::USERNAME;
                return url::HOST;
            }
            return url::PATH_PREFIX;
        }
        // t1 == PATH
        return t1;
    }();

    if (!need_save()) return;

    // copy flags; they can be used when copying / serializing url parts below
    unsigned mask = 0;
    for (int ind = t1; ind <= t2; ++ind) {
        mask |= url::kPartFlagMask[ind];
    }
    url_.flags_ = (url_.flags_ & ~mask) | (src.flags_ & mask);

    // copy parts & str
    if (ifirst <= t2) {
        int ilast = t2;
        for (; ilast >= ifirst; --ilast) {
            if (src.part_end_[ilast])
                break;
        }
        if (ifirst <= ilast) {
            // prepare buffer to append data
            // IMPORTANT: do before any url_ members modifications!
            std::string& norm_url = start_part(ifirst);

            // last part and url_.path_segment_count_
            std::size_t lastp_end = src.part_end_[ilast];
            if (pathOpFn && ilast == url::PATH) {
                std::size_t segment_count = src.path_segment_count_;
                // https://isocpp.org/wiki/faq/pointers-to-members
                // todo: use std::invoke (c++17)
                (src.*pathOpFn)(lastp_end, segment_count);
                url_.path_segment_count_ = segment_count;
            } else if (ifirst <= url::PATH && url::PATH <= ilast) {
                url_.path_segment_count_ = src.path_segment_count_;
            }
            // src
            const std::size_t offset = src.part_end_[ifirst - 1] + detail::kPartStart[ifirst];
            const char* const first = src.norm_url_.data() + offset;
            const char* const last = src.norm_url_.data() + lastp_end;
            // dest
            const auto delta = util::checked_diff<std::ptrdiff_t>(norm_url.length(), offset);
            // copy normalized url string from src
            norm_url.append(first, last);
            // adjust url_.part_end_
            for (int ind = ifirst; ind < ilast; ++ind) {
                // if (src.part_end_[ind]) // it is known, that src.part_end_[ind] has value, so check isn't needed
                url_.part_end_[ind] = src.part_end_[ind] + delta;
            }
            // ilast part from lastp
            url_.part_end_[ilast] = lastp_end + delta;
            last_pt_ = static_cast<url::PartType>(ilast);
        }
    }
}

// replace part in url

inline std::size_t url_serializer::get_part_pos(const url::PartType pt) const {
    return pt > url::SCHEME ? url_.part_end_[pt - 1] : 0;
}

inline std::size_t url_serializer::get_part_len(const url::PartType pt) const {
    return url_.part_end_[pt] - url_.part_end_[pt - 1];
}

inline void url_serializer::replace_part(const url::PartType new_pt, const char* str, const std::size_t len) {
    replace_part(new_pt, str, len, new_pt, 0);
}

inline void url_serializer::replace_part(const url::PartType last_pt, const char* str, const std::size_t len,
    const url::PartType first_pt, const std::size_t len0)
{
    const std::size_t b = get_part_pos(first_pt);
    const std::size_t l = url_.part_end_[last_pt] - b;
    url_.norm_url_.replace(b, l, str, len);
    std::fill(std::begin(url_.part_end_) + first_pt, std::begin(url_.part_end_) + last_pt, b + len0);
    // adjust positions
    const auto diff = util::checked_diff<std::ptrdiff_t>(len, l);
    if (diff) {
        for (auto it = std::begin(url_.part_end_) + last_pt; it != std::end(url_.part_end_); ++it) {
            if (*it == 0) break;
            // perform arithmetics using signed type ptrdiff_t, because diff can be negative
            *it = static_cast<std::ptrdiff_t>(*it) + diff;
        }
    }
}


// url_setter class

// inline url_setter::~url_setter() {}

//???
inline void url_setter::reserve(std::size_t new_cap) {
    strp_.reserve(new_cap);
}

// set scheme

inline std::string& url_setter::start_scheme() {
    return strp_;
}

inline void url_setter::save_scheme() {
    replace_part(url::SCHEME, strp_.data(), strp_.length());
    set_scheme(strp_.length());
}

// set/clear/empty url's part

inline std::string& url_setter::start_part(url::PartType new_pt) {
    assert(new_pt > url::SCHEME);
    curr_pt_ = new_pt;
    if (url_.part_end_[new_pt]) {
        // is there any part after new_pt?
        if (new_pt < url::FRAGMENT && url_.part_end_[new_pt] < url_.norm_url_.length()) {
            use_strp_ = true;
            switch (new_pt) {
            case url::HOST:
                if (get_part_len(url::SCHEME_SEP) < 3)
                    strp_ = "://";
                else
                    strp_.clear();
                break;
            case url::PASSWORD:
            case url::PORT:
                strp_ = ':';
                break;
            case url::QUERY:
                strp_ = '?';
                break;
            default:
                strp_.clear();
                break;
            }
            return strp_;
        }
        // Remove new_pt part
        last_pt_ = static_cast<url::PartType>(static_cast<int>(new_pt) - 1);
        url_.norm_url_.resize(url_.part_end_[last_pt_]);
        url_.part_end_[new_pt] = 0;
        // if there are empty parts after new_pt, then set their end positions to zero
        for (auto pt = static_cast<int>(new_pt) + 1; pt <= url::FRAGMENT && url_.part_end_[pt]; ++pt)
            url_.part_end_[pt] = 0;
    } else {
        last_pt_ = find_last_part(new_pt);
    }

    use_strp_ = false;
    return url_serializer::start_part(new_pt);
}

inline void url_setter::save_part() {
    if (use_strp_) {
        if (curr_pt_ == url::HOST) {
            if (get_part_len(url::SCHEME_SEP) < 3)
                // SCHEME_SEP, USERNAME, PASSWORD, HOST_START; HOST
                replace_part(url::HOST, strp_.data(), strp_.length(), url::SCHEME_SEP, 3);
            else
                replace_part(url::HOST, strp_.data(), strp_.length());
        } else {
            const bool empty_val = strp_.length() <= detail::kPartStart[curr_pt_];
            switch (curr_pt_) {
            case url::USERNAME:
            case url::PASSWORD:
                if (!empty_val && !has_credentials()) {
                    strp_ += '@';
                    // USERNAME, PASSWORD; HOST_START
                    replace_part(url::HOST_START, strp_.data(), strp_.length(), curr_pt_, strp_.length() - 1);
                    break;
                } else if (empty_val && is_empty(curr_pt_ == url::USERNAME ? url::PASSWORD : url::USERNAME)) {
                    // both username and password will be empty, so also drop '@'
                    replace_part(url::HOST_START, "", 0, curr_pt_, 0);
                    break;
                }
                UPA_FALLTHROUGH
            default:
                if ((curr_pt_ == url::PASSWORD || curr_pt_ == url::PORT) && empty_val)
                    strp_.clear(); // drop ':'
                replace_part(curr_pt_, strp_.data(), strp_.length());
                break;
            }
        }
        // cleanup
        strp_.clear();
    } else {
        url_serializer::save_part();
    }
}

inline void url_setter::clear_part(const url::PartType pt) {
    if (url_.part_end_[pt]) {
        replace_part(pt, "", 0);
        url_.flags_ &= ~(1u << pt); // set to null
    }
}

inline void url_setter::empty_part(const url::PartType pt) {
    if (url_.part_end_[pt]) {
        replace_part(pt, "", 0);
    }
}

inline void url_setter::empty_host() {
    empty_part(url::HOST);
    url_.set_host_type(HostType::Empty);
}

inline std::string& url_setter::start_path_segment() {
    //curr_pt_ = url::PATH; // not used
    strp_ += '/';
    return strp_;
}

inline void url_setter::save_path_segment() {
    path_seg_end_.push_back(strp_.length());
}

inline void url_setter::commit_path() {
    // fill part_end_ until url::PATH if not filled
    for (int ind = url::PATH; ind > 0; --ind) {
        if (url_.part_end_[ind]) break;
        url_.part_end_[ind] = url_.norm_url_.length();
    }
    // replace path part
    replace_part(url::PATH, strp_.data(), strp_.length());
    url_.path_segment_count_ = path_seg_end_.size();

    // "/." path prefix
    adjust_path_prefix();
}

// https://url.spec.whatwg.org/#shorten-a-urls-path

inline void url_setter::shorten_path() {
    if (path_seg_end_.size() == 1) {
        if (is_file_scheme() && strp_.length() == 3 &&
            detail::is_normalized_windows_drive(strp_[1], strp_[2]))
            return;
        path_seg_end_.pop_back();
        strp_.clear();
    } else if (path_seg_end_.size() >= 2) {
        path_seg_end_.pop_back();
        strp_.resize(path_seg_end_.back());
    }
}

inline bool url_setter::is_empty_path() const {
    assert(!url_.has_opaque_path());
    // path_seg_end_ has meaning only if path is a list (path isn't opaque)
    return path_seg_end_.empty();
}

// https://url.spec.whatwg.org/#potentially-strip-trailing-spaces-from-an-opaque-path

inline void url_setter::potentially_strip_trailing_spaces_from_an_opaque_path() {
    if (url_.has_opaque_path() &&
        is_null(url::FRAGMENT) &&
        is_null(url::QUERY)) {
        // Remove all trailing U+0020 SPACE code points from URL’s path.
        // Note. If the entire path consists of spaces or is empty, then last non-space
        // character in the url_.norm_url_ is scheme separator (:).
        const auto newlen = url_.norm_url_.find_last_not_of(' ') + 1;
        url_.norm_url_.resize(newlen);
        for (int ind = url::PATH; ind < url_.part_end_.size() && url_.part_end_[ind]; ++ind)
            url_.part_end_[ind] = newlen;
    }
}

inline url::PartType url_setter::find_last_part(url::PartType pt) const {
    for (int ind = pt; ind > 0; --ind)
        if (url_.part_end_[ind])
            return static_cast<url::PartType>(ind);
    return url::SCHEME;
}

/// @brief Check UNC path
///
/// Input - path string with the first two backslashes skipped
///
/// @param[in] first start of path string
/// @param[in] last end of path string
/// @return pointer to the end of the UNC share name, or `nullptr`
///   if input is not valid UNC
template <typename CharT>
inline const CharT* is_unc_path(const CharT* first, const CharT* last)
{
    // https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-dfsc/149a3039-98ce-491a-9268-2f5ddef08192
    std::size_t path_components_count = 0;
    const CharT* end_of_share_name = nullptr;
    const auto* start = first;
    while (start != last) {
        const auto* pcend = std::find_if(start, last, detail::is_windows_slash<CharT>);
        // path components MUST be at least one character in length
        if (start == pcend)
            return nullptr;
        // path components MUST NOT contain a backslash (\) or a null
        if (std::find(start, pcend, '\0') != pcend)
            return nullptr;

        ++path_components_count;

        switch (path_components_count) {
        case 1:
            // Check the first UNC path component (hostname)
            switch (pcend - start) {
            case 1:
                // Do not allow "?" and "." hostnames, because "\\?\" means Win32 file
                // namespace and "\\.\" means Win32 device namespace
                if (start[0] == '?' || start[0] == '.')
                    return nullptr;
                break;
            case 2:
                // Do not allow Windows drive letter, because it is not a valid hostname
                if (detail::is_windows_drive(start[0], start[1]))
                    return nullptr;
                break;
            }
            // Accept UNC path with hostname, even if it does not contain share-name
            end_of_share_name = pcend;
            break;
        case 2:
            // Check the second UNC path component (share name).
            // Do not allow "." and ".." as share names, because they have
            // a special meaning and are removed by the URL parser.
            switch (pcend - start) {
            case 1:
                if (start[0] == '.')
                    return nullptr;
                break;
            case 2:
                if (start[0] == '.' && start[1] == '.')
                    return nullptr;
                break;
            }
            // A valid UNC path MUST contain two or more path components
            end_of_share_name = pcend;
            break;
        default:;
        }
        if (pcend == last) break;
        start = pcend + 1; // skip '\'
    }
    return end_of_share_name;
}

/// @brief Check path contains ".." segment
///
/// @param[in] first start of path string
/// @param[in] last end of path string
/// @param[in] is_slash function to check char is slash (or backslash)
/// @return true if path contains ".." segment
template <typename CharT, typename IsSlash>
inline bool has_dot_dot_segment(const CharT* first, const CharT* last, IsSlash is_slash) {
    if (last - first >= 2) {
        const auto* ptr = first;
        const auto* end = last - 1;
        while ((ptr = std::char_traits<CharT>::find(ptr, end - ptr, '.')) != nullptr) {
            if (ptr[1] == '.' &&
                (ptr == first || is_slash(*(ptr - 1))) &&
                (last - ptr == 2 || is_slash(ptr[2])))
                return true;
            // skip '.' and following char
            ptr += 2;
            if (ptr >= end)
                break;
        }
    }
    return false;
}

} // namespace detail


// URL utilities (non-member functions)

/// @brief URL equivalence
///
/// Determines if @a lhs equals to @a rhs, optionally with an @a exclude_fragments flag.
/// More info: https://url.spec.whatwg.org/#concept-url-equals
///
/// @param[in] lhs,rhs URLs to compare
/// @param[in] exclude_fragments exclude fragments when comparing
inline bool equals(const url& lhs, const url& rhs, bool exclude_fragments = false) {
    return lhs.serialize(exclude_fragments) == rhs.serialize(exclude_fragments);
}

/// @brief Lexicographically compares two URL's
inline bool operator==(const url& lhs, const url& rhs) noexcept {
    return lhs.norm_url_ == rhs.norm_url_;
}

/// @brief Swaps the contents of two URLs
///
/// Swaps the contents of the @a lhs and @a rhs URLs
///
/// @param[in,out] lhs
/// @param[in,out] rhs
inline void swap(url& lhs, url& rhs) UPA_NOEXCEPT_17 {
    lhs.swap(rhs);
}

/// @brief File path format
enum class file_path_format {
    posix = 1,  ///< POSIX file path format
    windows,    ///< Windows file path format
#ifdef _WIN32
    native = windows ///< The file path format corresponds to the OS on which the code was compiled
#else
    native = posix   ///< The file path format corresponds to the OS on which the code was compiled
#endif
};

/// @brief Make URL from OS file path
///
/// The file path must be absolute and must not contain any dot-dot (..)
/// segments.
///
/// There is a difference in how paths with dot-dot segments are normalized in the OS and in the
/// WHATWG URL standard. For example, in POSIX the path `/a//../b` is normalized to `/b`, while
/// the URL parser normalizes this path to `/a/b`. This library does not implement OS specific path
/// normalization, which is the main reason why it does not accept paths with dot-dot segments.
/// Therefore, if there are such segments in the path, it should be normalized by OS tools before
/// being submitted to this function. Normalization can be done using the POSIX `realpath`
/// function, the Windows `GetFullPathName` function, or, if you are using C++17, the
/// `std::filesystem::canonical` function.
///
/// Throws url_error exception on error.
///
/// @param[in] str absolute file path string
/// @param[in] format file path format, one of upa::file_path_format::posix,
///   upa::file_path_format::windows, upa::file_path_format::native
/// @return file URL
/// @see [Pathname (POSIX)](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap03.html#tag_03_254),
///   [realpath](https://pubs.opengroup.org/onlinepubs/9799919799/functions/realpath.html),
///   [GetFullPathName](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamew),
///   [std::filesystem::canonical](https://en.cppreference.com/w/cpp/filesystem/canonical)
template <class StrT, enable_if_str_arg_t<StrT> = 0>
inline url url_from_file_path(StrT&& str, file_path_format format = file_path_format::native) {
    using CharT = str_arg_char_t<StrT>;
    const auto inp = make_str_arg(std::forward<StrT>(str));
    const auto* first = inp.begin();
    const auto* last = inp.end();

    if (first == last) {
        throw url_error(validation_errc::file_empty_path, "Empty file path");
    }

    const auto* pointer = first;
    const auto* start_of_check = first;
    const code_point_set* no_encode_set = nullptr;

    std::string str_url("file://");

    if (format == file_path_format::posix) {
        if (!detail::is_posix_slash(*first))
            throw url_error(validation_errc::file_unsupported_path, "Non-absolute POSIX path");
        if (detail::has_dot_dot_segment(start_of_check, last, detail::is_posix_slash<CharT>))
            throw url_error(validation_errc::file_unsupported_path, "Unsupported file path");
        // Absolute POSIX path
        no_encode_set = &posix_path_no_encode_set;
    } else {
        // Windows path?
        bool is_unc = false;

        // https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file
        // https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats
        // https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
        if (last - pointer >= 2 &&
            detail::is_windows_slash(pointer[0]) &&
            detail::is_windows_slash(pointer[1])) {
            pointer += 2; // skip '\\'

            // It is Win32 namespace path or UNC path?
            if (last - pointer >= 2 &&
                (pointer[0] == '?' || pointer[0] == '.') &&
                detail::is_windows_slash(pointer[1])) {
                // Win32 File ("\\?\") or Device ("\\.\") namespace path
                pointer += 2; // skip "?\" or ".\"
                if (last - pointer >= 4 &&
                    (pointer[0] | 0x20) == 'u' &&
                    (pointer[1] | 0x20) == 'n' &&
                    (pointer[2] | 0x20) == 'c' &&
                    detail::is_windows_slash(pointer[3])) {
                    pointer += 4; // skip "UNC\"
                    is_unc = true;
                }
            } else {
                // UNC path
                is_unc = true;
            }
        }
        start_of_check = is_unc
            ? detail::is_unc_path(pointer, last)
            : detail::is_windows_os_drive_absolute_path(pointer, last);
        if (start_of_check == nullptr ||
            detail::has_dot_dot_segment(start_of_check, last, detail::is_windows_slash<CharT>))
            throw url_error(validation_errc::file_unsupported_path, "Unsupported file path");
        no_encode_set = &raw_path_no_encode_set;
        if (!is_unc) str_url.push_back('/'); // start path
    }

    // Check for null characters
    if (util::contains_null(start_of_check, last))
        throw url_error(validation_errc::null_character, "Path contains null character");

    // make URL
    detail::append_utf8_percent_encoded(pointer, last, *no_encode_set, str_url);
    return url(str_url);
}

/// @brief Get OS path from file URL
///
/// Throws url_error exception on error.
///
/// @param[in] file_url file URL
/// @param[in] format file path format, one of upa::file_path_format::posix,
///   upa::file_path_format::windows, upa::file_path_format::native
/// @return OS path
inline std::string path_from_file_url(const url& file_url, file_path_format format = file_path_format::native) {
    if (!file_url.is_file_scheme())
        throw url_error(validation_errc::not_file_url, "Not a file URL");

    // source
    const auto hostname = file_url.hostname();
    const bool is_host = !hostname.empty();

    // target
    std::string path;

    if (format == file_path_format::posix) {
        if (is_host)
            throw url_error(validation_errc::file_url_cannot_have_host, "POSIX path cannot have host");
        // percent decode pathname
        detail::append_percent_decoded(file_url.pathname(), path);
    } else {
        // format == file_path_format::windows
        if (is_host) {
            // UNC path cannot have "." hostname, because "\\.\" means Win32 device namespace
            if (hostname == ".")
                throw url_error(validation_errc::file_url_unsupported_host, "UNC path cannot have \".\" hostname");
            // UNC path
            path.append("\\\\");
            if (file_url.host_type() == HostType::IPv6) {
                // Form an IPV6 address host-name by substituting hyphens for the colons and appending ".ipv6-literal.net"
                // https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/62e862f4-2a51-452e-8eeb-dc4ff5ee33cc
                std::replace_copy(std::next(hostname.begin()), std::prev(hostname.end()),
                    std::back_inserter(path), ':', '-');
                path.append(".ipv6-literal.net");
            } else {
                path.append(hostname);
            }
        }

        // percent decode pathname and normalize slashes
        const auto start = static_cast<std::ptrdiff_t>(path.length());
        detail::append_percent_decoded(file_url.pathname(), path);
        std::replace(std::next(path.begin(), start), path.end(), '/', '\\');

        if (is_host) {
            if (!detail::is_unc_path(path.data() + 2, path.data() + path.length()))
                throw url_error(validation_errc::file_url_invalid_unc, "Invalid UNC path");
        } else {
            if (detail::pathname_has_windows_os_drive(path)) {
                path.erase(0, 1); // remove leading '\\'
                if (path.length() == 2)
                    path.push_back('\\'); // "C:" -> "C:\"
            } else {
                // https://datatracker.ietf.org/doc/html/rfc8089#appendix-E.3.2
                // Maybe a UNC path. Possible variants:
                // 1) file://///host/path -> \\\host\path
                // 2) file:////host/path -> \\host\path
                const auto count_leading_slashes = std::find_if(
                    path.data(),
                    path.data() + std::min(static_cast<std::size_t>(4), path.length()),
                    [](char c) { return c != '\\'; }) - path.data();
                if (count_leading_slashes == 3)
                    path.erase(0, 1); // remove leading '\\'
                else if (count_leading_slashes != 2)
                    throw url_error(validation_errc::file_url_not_windows_path, "Not a Windows path");
                if (!detail::is_unc_path(path.data() + 2, path.data() + path.length()))
                    throw url_error(validation_errc::file_url_invalid_unc, "Invalid UNC path");
            }
        }
    }

    // Check for null characters
    if (util::contains_null(path.begin(), path.end()))
        throw url_error(validation_errc::null_character, "Path contains null character");

    return path;
}


} // namespace upa


namespace std {

/// @brief std::hash specialization for upa::url class
template<>
struct hash<upa::url> {
    std::size_t operator()(const upa::url& url) const noexcept {
        return std::hash<std::string>{}(url.norm_url_);
    }
};

} // namespace std

// Includes that require the url class declaration
#include "url_search_params-inl.h"

#endif // UPA_URL_H
