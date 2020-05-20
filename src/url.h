// Copyright 2016-2019 Rimas Misevičius
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

#ifndef WHATWG_URL_H
#define WHATWG_URL_H

#include "config.h"
#include "buffer.h"
#include "int_cast.h"
#include "str_arg.h"
#include "url_host.h"
#include "url_idna.h"
#include "url_percent_encode.h"
#include "url_result.h"
#include "url_search_params.h"
#include "url_utf.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

// not yet
#define WHATWG_URL_USE_ENCODING 0

namespace whatwg {

// URL class

class url {
public:
    // types
    using str_view_type = url_str_view_t;

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

    /// Default constructor.
    ///
    /// Constructs empty URL.
    url()
        : scheme_inf_(nullptr)
        , flags_(INITIAL_FLAGS)
        , path_segment_count_(0)
        , part_end_({})
    {}

    /// Copy constructor.
    ///
    /// @param[in] other  URL to copy from
    url(const url& other) = default;

    /// Move constructor.
    ///
    /// Constructs the URL with the contents of @a other using move semantics.
    ///
    /// @param[in,out] other  URL to move to this object
    url(url&& other) noexcept = default;

    /// Copy assignment.
    ///
    /// @param[in] other  URL to copy from
    url& operator=(const url& other) = default;

    /// Move assignment.
    ///
    /// Replaces the contents with those of @a other using move semantics.
    ///
    /// @param[in,out] other  URL to move to this object
    url& operator=(url&& other) = default;

    /// Parsing constructor.
    ///
    /// Throws @a url_error exception on parse error.
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] pbase   pointer to base URL, may be @a nullptr
    template <class T, enable_if_str_arg_t<T> = 0>
    explicit url(T&& str_url, const url* pbase = nullptr);

    /// Parsing constructor.
    ///
    /// Throws @a url_error exception on parse error.
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] base    base URL
    template <class T, enable_if_str_arg_t<T> = 0>
    explicit url(T&& str_url, const url& base);

    /// Parsing constructor.
    ///
    /// Throws @a url_error exception on parse error.
    ///
    /// @param[in] str_url  URL string to parse
    /// @param[in] str_base base URL string
    template <class T, class TB, enable_if_str_arg_t<T> = 0, enable_if_str_arg_t<TB> = 0>
    explicit url(T&& str_url, TB&& str_base);

    /// Clear URL - make it empty.
    void clear();

    // Parser

    /// Parses given URL string against base URL.
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] base    pointer to base URL, may be nullptr
    /// @returns error code (@a url_result::Ok on success)
    template <class T, enable_if_str_arg_t<T> = 0>
    url_result parse(T&& str_url, const url* base) {
        const auto inp = make_str_arg(std::forward<T>(str_url));
        return do_parse(inp.begin(), inp.end(), base);
    }

    template <typename T1, typename T2, enable_if_str_arg_t<T1, T2> = 0>
    url_result parse(T1&& arg1, T2&& arg2, const url* base) {
        const auto inp = make_str_arg(std::forward<T1>(arg1), std::forward<T2>(arg2));
        return do_parse(inp.begin(), inp.end(), base);
    }

    // setters

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    bool href(Args&&... args);

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    bool protocol(Args&&... args);

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    bool username(Args&&... args);

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    bool password(Args&&... args);

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    bool host(Args&&... args);

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    bool hostname(Args&&... args);

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    bool port(Args&&... args);

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    bool pathname(Args&&... args);

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    bool search(Args&&... args);

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    bool hash(Args&&... args);

    // getters

    // get serialized URL
    str_view_type href() const;

    // ASCII serialized origin
    std::string origin() const;

    str_view_type protocol() const;

    str_view_type username() const;
    str_view_type password() const;

    str_view_type host() const;
    str_view_type hostname() const;
    HostType host_type() const;

    str_view_type port() const;

    // port to int
    int port_int() const;
    int real_port_int() const;

    // pathname + search
    str_view_type path() const;

    str_view_type pathname() const;

    str_view_type search() const;

    str_view_type hash() const;

    // https://url.spec.whatwg.org/#dom-url-searchparams
    url_search_params& searchParams();

    /// URL serializing
    ///
    /// Returns serialized URL in a string_view as defined here:
    /// https://url.spec.whatwg.org/#concept-url-serializer
    ///
    /// @param[in] exclude_fragment exclude fragment when serializing
    /// @returns serialized URL as string_view
    str_view_type serialize(bool exclude_fragment = false) const;

    // Get url info

    str_view_type get_part_view(PartType t) const;

    bool is_empty(const PartType t) const;
    bool is_null(const PartType t) const;

    bool is_special_scheme() const;
    bool is_file_scheme() const;

    // URL includes credentials?
    bool has_credentials() const;

    // stringify
    std::string to_string() const;

protected:
    struct scheme_info {
        str_view_type scheme;
        int default_port;           // -1 if none
        unsigned is_special : 1;    // "ftp", "file", "http", "https", "ws", "wss"
        unsigned is_file : 1;       // "file"
        unsigned is_ws : 1;         // "ws", "wss"

        // for search operations
        operator const str_view_type&() const { return scheme; }
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
        CANNOT_BE_BASE_FLAG = (1u << (PART_COUNT + 0)),
        // host type
        HOST_TYPE_SHIFT = (PART_COUNT + 1),
        HOST_TYPE_MASK = (7u << HOST_TYPE_SHIFT),

        // initial flags (empty (but not null) parts)
        // https://url.spec.whatwg.org/#url-representation
        INITIAL_FLAGS = SCHEME_FLAG | USERNAME_FLAG | PASSWORD_FLAG | PATH_FLAG,
    };

    // parsing constructor
    template <class T, enable_if_str_arg_t<T> = 0>
    explicit url(T&& str_url, const url* base, const char* what_arg);

    // parser
    template <typename CharT>
    url_result do_parse(const CharT* first, const CharT* last, const url* base);

    // get scheme info
    static const scheme_info kSchemes[];
    static const scheme_info* get_scheme_info(str_view_type src);

    // set scheme
    template <class StringT>
    void set_scheme_str(const StringT str);
    void set_scheme(const url& src);
    void set_scheme(const str_view_type str);
    void set_scheme(std::size_t end_of_scheme);
    void clear_scheme();

    // path util
    str_view_type get_path_first_string(std::size_t len) const;
    // path shortening
    bool get_path_rem_last(std::size_t& path_end, std::size_t& path_segment_count) const;
    bool get_shorten_path(std::size_t& path_end, std::size_t& path_segment_count) const;

    // flags
    void set_flag(const UrlFlag flag);

    bool cannot_be_base() const;
    void set_cannot_be_base();

    void set_host_type(const HostType ht);

    // info
    bool canHaveUsernamePasswordPort();

    // search params
    void clear_search_params();
    void parse_search_params();

private:
    std::string norm_url_;
    std::array<std::size_t, PART_COUNT> part_end_;
    const scheme_info* scheme_inf_;
    unsigned flags_;
    std::size_t path_segment_count_;
    url_search_params_ptr search_params_ptr_;

    friend class url_serializer;
    friend class url_setter;
    friend class url_parser;
    friend class url_search_params;
};

/// URL equivalence
///
/// Determines if @a lhs equals to @a rhs, optionally with an @a exclude_fragments flag.
/// More info: https://url.spec.whatwg.org/#concept-url-equals
///
/// @param[in] lhs,rhs URLs to compare
/// @param[in] exclude_fragments exclude fragments when comparing
inline bool equals(const url& lhs, const url& rhs, bool exclude_fragments = false) {
    return lhs.serialize(exclude_fragments) == rhs.serialize(exclude_fragments);
}


class url_serializer : public host_output {
public:
    // types
    using str_view_type = url::str_view_type;

    url_serializer(url& dest_url)
        : url_(dest_url)
        , last_pt_(url::SCHEME)
    {}

    ~url_serializer();

    void new_url() { url_.clear(); }
    virtual void reserve(std::size_t new_cap) { url_.norm_url_.reserve(new_cap); }

    // set data
    void set_scheme(const url& src) { url_.set_scheme(src); }
    void set_scheme(const str_view_type str) { url_.set_scheme(str); }
    void set_scheme(std::size_t end_of_scheme) { url_.set_scheme(end_of_scheme); }

    // set/clear scheme
    virtual std::string& start_scheme();
    virtual void save_scheme();
    virtual void clear_scheme();

    // set url's part
    void fill_parts_offset(url::PartType t1, url::PartType t2, std::size_t offset);
    virtual std::string& start_part(url::PartType new_pt);
    virtual void save_part();

    // TODO: can be used instead of clear_host():
    virtual void clear_part(const url::PartType /*pt*/) {}

    virtual void clear_host();
    virtual void empty_host();

    // host_output overrides
    std::string& hostStart() override;
    void hostDone(HostType ht) override;

    // path
    // TODO: append_to_path() --> append_empty_to_path()
    void append_to_path();
    virtual std::string& start_path_segment();
    virtual void save_path_segment();
    virtual void commit_path();
    // if '/' not required:
    std::string& start_path_string();
    void save_path_string();

    virtual void shorten_path();
    // retunrs how many slashes are removed
    virtual std::size_t remove_leading_path_slashes();

    typedef bool (url::*PathOpFn)(std::size_t& path_end, std::size_t& segment_count) const;
    void append_parts(const url& src, url::PartType t1, url::PartType t2, PathOpFn pathOpFn = nullptr);

    // flags
    void set_flag(const url::UrlFlag flag) { url_.set_flag(flag); }
    void set_host_type(const HostType ht) { url_.set_host_type(ht); }
    // IMPORTANT: cannot-be-a-base-URL flag must be set before or just after
    // SCHEME set; because other part's serialization depends on this flag
    void set_cannot_be_base() {
        assert(last_pt_ == url::SCHEME);
        url_.set_cannot_be_base();
    }

    // get info
    str_view_type get_part_view(url::PartType t) const { return url_.get_part_view(t); }
    bool is_empty(const url::PartType t) const { return url_.is_empty(t); }
    virtual bool is_empty_path() const { return url_.path_segment_count_ == 0; }
    bool is_null(const url::PartType t) const  { return url_.is_null(t); }
    bool is_special_scheme() const { return url_.is_special_scheme(); }
    bool is_file_scheme() const { return url_.is_file_scheme(); }
    bool has_credentials() const { return url_.has_credentials(); }
    const url::scheme_info* scheme_inf() const { return url_.scheme_inf_; }
    int port_int() const { return url_.port_int(); }

#if 0
    std::string& start_username();
    std::string& start_password();
    std::string& start_host();
    std::string& start_port();
    std::string& start_path();
    std::string& start_query();
    std::string& start_fragment();

    void save_username();
    void save_password();
    void save_host();
    void save_port();
    void save_path();
    void save_query();
    void save_fragment();
#endif

protected:
    std::size_t get_part_pos(const url::PartType pt) const;
    std::size_t get_part_len(const url::PartType pt) const;
    void replace_part(const url::PartType new_pt, const char* str, const std::size_t len);
    void replace_part(const url::PartType last_pt, const char* str, const std::size_t len,
        const url::PartType first_pt, const std::size_t len0);

protected:
    url& url_;
    url::PartType last_pt_;
};

// TODO
class url_setter : public url_serializer {
public:
    url_setter(url& dest_url)
        : url_serializer(dest_url)
        , use_strp_(true)
        , curr_pt_(url::SCHEME)
    {}

    ~url_setter();

    //???
    void reserve(std::size_t new_cap) override;

    // set/clear scheme
    std::string& start_scheme() override;
    void save_scheme() override;
    void clear_scheme() override;

    // set/clear/empty url's part
    std::string& start_part(url::PartType new_pt) override;
    void save_part() override;

    void clear_part(const url::PartType pt) override;
    void empty_part(const url::PartType pt); // override

    void clear_host() override { clear_part(url::HOST); }
    void empty_host() override { empty_part(url::HOST); }

    // path
    std::string& start_path_segment() override;
    void save_path_segment() override;
    void commit_path() override;

    void shorten_path() override;
    // retunrs how many slashes are removed
    std::size_t remove_leading_path_slashes() override;
    bool is_empty_path() const override;

protected:
    url::PartType find_last_part(url::PartType pt) const;

#if 0
    void insert_part(url::PartType new_pt, const char* str, std::size_t len);
#endif

protected:
    bool use_strp_;
    std::string strp_;
    std::vector<std::size_t> path_seg_end_;
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
        cannot_be_base_URL_path_state,
        query_state,
        fragment_state
    };

    template <typename CharT>
    static url_result url_parse(url_serializer& urls, const CharT* first, const CharT* last, const url* base, State state_override = not_set_state);

    template <typename CharT>
    static url_result parse_host(url_serializer& urls, const CharT* first, const CharT* last);

    template <typename CharT>
    static void parse_path(url_serializer& urls, const CharT* first, const CharT* last);

private:
    template <typename CharT>
    static bool do_path_segment(const CharT* pointer, const CharT* last, std::string& output);

    template <typename CharT>
    static bool do_simple_path(const CharT* pointer, const CharT* last, std::string& output);
};


namespace detail {

// url_error what() values
extern const char kURLParseError[];
extern const char kBaseURLParseError[];

// canonical version of each possible input letter in the scheme
extern const char kSchemeCanonical[0x80];

// part start
extern const uint8_t kPartStart[url::PART_COUNT];

inline int port_from_str(const char* first, const char* last) {
    int port = 0;
    for (auto it = first; it != last; ++it) {
        port = port * 10 + (*it - '0');
    }
    return port;
}

// Removable URL chars

template <typename CharT, typename UCharT = typename std::make_unsigned<CharT>::type>
auto to_unsigned(CharT ch) -> UCharT {
    return static_cast<UCharT>(ch);
}

// chars to trim (C0 control or space: U+0000 to U+001F or U+0020)
template <typename CharT>
inline bool is_trim_char(CharT ch) {
    return to_unsigned(ch) <= ' ';
}

// chars what should be removed from the URL (ASCII tab or newline: U+0009, U+000A, U+000D)
template <typename CharT>
inline bool is_removable_char(CharT ch) {
    return ch == '\r' || ch == '\n' || ch == '\t';
}

template <typename CharT>
inline void do_trim(const CharT*& first, const CharT*& last) {
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
InputIt find_last(InputIt first, InputIt last, const T& value) {
    for (auto it = last; it > first;) {
        --it;
        if (*it == value) return it;
    }
    return last;
}

// special chars

template <typename CharT>
inline bool is_slash(CharT ch) {
    return ch == '/' || ch == '\\';
}

template <typename CharT>
inline bool is_ascii_digit(CharT ch) {
    return ch <= '9' && ch >= '0';
}

template <typename CharT>
inline bool is_ascii_alpha(CharT ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

// Scheme chars

template <typename CharT>
inline bool is_first_scheme_char(CharT ch) {
    return is_ascii_alpha(ch);
}

template <typename CharT>
inline bool is_authority_end_char(CharT c) {
    return c == '/' || c == '?' || c == '#';
}

template <typename CharT>
inline bool is_special_authority_end_char(CharT c) {
    return c == '/' || c == '?' || c == '#' || c == '\\';
}

// Windows drive letter

// https://url.spec.whatwg.org/#windows-drive-letter

template <typename CharT>
inline bool is_Windows_drive(CharT c1, CharT c2) {
    return is_ascii_alpha(c1) && (c2 == ':' || c2 == '|');
}

template <typename CharT>
inline bool is_normalized_Windows_drive(CharT c1, CharT c2) {
    return is_ascii_alpha(c1) && c2 == ':';
}

// https://url.spec.whatwg.org/#start-with-a-windows-drive-letter
template <typename CharT>
inline bool starts_with_Windows_drive(const CharT* pointer, const CharT* last) {
    const auto length = last - pointer;
#if 1
    return
        (length == 2 || (length > 2 && detail::is_special_authority_end_char(pointer[2]))) &&
        detail::is_Windows_drive(pointer[0], pointer[1]);
#else
    return
        length >= 2 &&
        detail::is_Windows_drive(pointer[0], pointer[1]) &&
        (length == 2 || detail::is_special_authority_end_char(pointer[2]));
#endif
}

} // namespace detail


// url class

inline url::str_view_type url::href() const {
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
    } else if (get_part_view(SCHEME) == url::str_view_type{ "blob", 4 }) {
        // Warning: this library does not support blob URL store, so it allways assumes
        // URL's blob URL entry is null and retrieves origin from the URL's path.
        url u;
        if (u.parse(get_part_view(PATH), nullptr) == url_result::Ok)
            return u.origin();
    }
    return "null"; // opaque origin
}

inline url::str_view_type url::protocol() const {
    // "scheme:"
    return str_view_type(norm_url_.data(), part_end_[SCHEME] ? part_end_[SCHEME] + 1 : 0);
}

inline url::str_view_type url::username() const {
    return get_part_view(USERNAME);
}

inline url::str_view_type url::password() const {
    return get_part_view(PASSWORD);
}

inline url::str_view_type url::host() const {
    if (is_null(HOST))
        return str_view_type();
    // "hostname:port"
    const std::size_t b = part_end_[HOST_START];
    const std::size_t e = is_null(PORT) ? part_end_[HOST] : part_end_[PORT];
    return str_view_type(norm_url_.data() + b, e - b);
}

inline url::str_view_type url::hostname() const {
    return get_part_view(HOST);
}

inline HostType url::host_type() const {
    return static_cast<HostType>((flags_ & HOST_TYPE_MASK) >> HOST_TYPE_SHIFT);
}

inline url::str_view_type url::port() const {
    return get_part_view(PORT);
}

inline int url::port_int() const {
    auto vport = get_part_view(PORT);
    return vport.length() ? detail::port_from_str(vport.data(), vport.data() + vport.length()) : -1;
}

inline int url::real_port_int() const {
    auto vport = get_part_view(PORT);
    if (vport.length()) {
        return detail::port_from_str(vport.data(), vport.data() + vport.length());
    } else {
        return scheme_inf_ ? scheme_inf_->default_port : -1;
    }
}

// pathname + search
inline url::str_view_type url::path() const {
    // "pathname?query"
    const std::size_t b = part_end_[PATH - 1];
    const std::size_t e = part_end_[QUERY] ? part_end_[QUERY] : part_end_[PATH];
    return str_view_type(norm_url_.data() + b, e ? e - b : 0);
}

inline url::str_view_type url::pathname() const {
    // https://url.spec.whatwg.org/#dom-url-pathname
    // already serialized as needed
    return get_part_view(PATH);
}

inline url::str_view_type url::search() const {
    if (is_empty(QUERY))
        return str_view_type();
    // return with '?'
    const std::size_t b = part_end_[QUERY - 1];
    const std::size_t e = part_end_[QUERY];
    return str_view_type(norm_url_.data() + b, e - b);
}

inline url::str_view_type url::hash() const {
    if (is_empty(FRAGMENT))
        return str_view_type();
    // return with '#'
    const std::size_t b = part_end_[FRAGMENT - 1];
    const std::size_t e = part_end_[FRAGMENT];
    return str_view_type(norm_url_.data() + b, e - b);
}

inline url_search_params& url::searchParams() {
    if (!search_params_ptr_)
        search_params_ptr_.init(this);
    return *search_params_ptr_;
}

inline void url::clear_search_params() {
    if (search_params_ptr_)
        search_params_ptr_.clear_params();
}

inline void url::parse_search_params() {
    if (search_params_ptr_)
        search_params_ptr_.parse_params(get_part_view(QUERY));
}

inline url::str_view_type url::serialize(bool exclude_fragment) const {
    if (exclude_fragment && part_end_[FRAGMENT])
        return str_view_type(norm_url_.data(), part_end_[QUERY]);
    else
        return norm_url_;
}

// Get url info

inline url::str_view_type url::get_part_view(PartType t) const {
    if (t == SCHEME)
        return str_view_type(norm_url_.data(), part_end_[SCHEME]);
    // begin & end offsets
    const std::size_t b = part_end_[t - 1] + detail::kPartStart[t];
    const std::size_t e = part_end_[t];
    return str_view_type(norm_url_.data() + b, e > b ? e - b : 0);
}

inline bool url::is_empty(const PartType t) const {
    if (t == SCHEME)
        return part_end_[SCHEME] == 0;
    // begin & end offsets
    const std::size_t b = part_end_[t - 1] + detail::kPartStart[t];
    const std::size_t e = part_end_[t];
    return b >= e;
}

inline bool url::is_null(const PartType t) const {
    return !(flags_ & (1u << t));
}

inline bool url::is_special_scheme() const {
    return scheme_inf_ && scheme_inf_->is_special;
}

inline bool url::is_file_scheme() const {
    return scheme_inf_ && scheme_inf_->is_file;
}

inline bool url::has_credentials() const {
    return !is_empty(USERNAME) || !is_empty(PASSWORD);
}

// set scheme

template <class StringT>
inline void url::set_scheme_str(const StringT str) {
    norm_url_.resize(0); // clear all
    part_end_[SCHEME] = str.length();
    norm_url_.append(str.data(), str.length());
    norm_url_ += ':';
}

inline void url::set_scheme(const url& src) {
    set_scheme_str(src.get_part_view(SCHEME));
    scheme_inf_ = src.scheme_inf_;
}

inline void url::set_scheme(const str_view_type str) {
    set_scheme_str(str);
    scheme_inf_ = get_scheme_info(str);
}

inline void url::set_scheme(std::size_t end_of_scheme) {
    part_end_[SCHEME] = end_of_scheme;
    scheme_inf_ = get_scheme_info(get_part_view(SCHEME));
}

inline void url::clear_scheme() {
    norm_url_.resize(0); // clear all
    part_end_[SCHEME] = 0;
    //?TODO?: part_end_[SCHEME_SEP] = 0;
    scheme_inf_ = nullptr;
}

// flags

inline void url::set_flag(const UrlFlag flag) {
    flags_ |= flag;
}

inline bool url::cannot_be_base() const {
    return !!(flags_ & CANNOT_BE_BASE_FLAG);
}

inline void url::set_cannot_be_base() {
    set_flag(CANNOT_BE_BASE_FLAG);
}

inline void url::set_host_type(const HostType ht) {
    flags_ = (flags_ & ~HOST_TYPE_MASK) | HOST_FLAG | (static_cast<unsigned int>(ht) << HOST_TYPE_SHIFT);
}

inline bool url::canHaveUsernamePasswordPort() {
    return !(is_empty(url::HOST) || cannot_be_base() || is_file_scheme());
}

// url parsing

template <class T, enable_if_str_arg_t<T>>
inline url::url(T&& str_url, const url* pbase)
    : url(std::forward<T>(str_url), pbase, detail::kURLParseError)
{}

template <class T, enable_if_str_arg_t<T>>
inline url::url(T&& str_url, const url& base)
    : url(std::forward<T>(str_url), &base, detail::kURLParseError)
{}

template <class T, class TB, enable_if_str_arg_t<T>, enable_if_str_arg_t<TB>>
inline url::url(T&& str_url, TB&& str_base)
    : url(std::forward<T>(str_url), url(std::forward<TB>(str_base), nullptr, detail::kBaseURLParseError))
{}

template <class T, enable_if_str_arg_t<T>>
inline url::url(T&& str_url, const url* base, const char* what_arg) {
    const auto inp = make_str_arg(std::forward<T>(str_url));
    const auto res = do_parse(inp.begin(), inp.end(), base);
    if (res != url_result::Ok)
        throw url_error(res, what_arg);
}

inline void url::clear() {
    norm_url_.resize(0);
    part_end_.fill(0);
    scheme_inf_ = nullptr;
    flags_ = INITIAL_FLAGS;
    path_segment_count_ = 0;
}

template <typename CharT>
inline url_result url::do_parse(const CharT* first, const CharT* last, const url* base) {
    url_result res;
    {
        url_serializer urls(*this); // new URL

        // reset URL
        urls.new_url();

        // remove any leading and trailing C0 control or space:
        detail::do_trim(first, last);
        //TODO-WARN: validation error if trimmed

        res = url_parser::url_parse(urls, first, last, base);
    }
    parse_search_params();
    return res;
}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline bool url::href(Args&&... args) {
    url u; // parsedURL

    const auto inp = make_str_arg(std::forward<Args>(args)...);
    if (u.do_parse(inp.begin(), inp.end(), nullptr) == url_result::Ok) {
        *this = std::move(u);
        return true;
    }
    return false;
}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline bool url::protocol(Args&&... args) {
    url_setter urls(*this);

    const auto inp = make_str_arg(std::forward<Args>(args)...);
    return url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, url_parser::scheme_start_state) == url_result::Ok;
}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline bool url::username(Args&&... args) {
    if (canHaveUsernamePasswordPort()) {
        url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<Args>(args)...);

        std::string& str_username = urls.start_part(url::USERNAME);
        // UTF-8 percent encode it using the userinfo encode set
        detail::AppendStringOfType(inp.begin(), inp.end(), detail::CHAR_USERINFO, str_username);
        urls.save_part();
        return true;
    }
    return false;
}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline bool url::password(Args&&... args) {
    if (canHaveUsernamePasswordPort()) {
        url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<Args>(args)...);

        std::string& str_password = urls.start_part(url::PASSWORD);
        // UTF-8 percent encode it using the userinfo encode set
        detail::AppendStringOfType(inp.begin(), inp.end(), detail::CHAR_USERINFO, str_password);
        urls.save_part();
        return true;
    }
    return false;
}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline bool url::host(Args&&... args) {
    if (!cannot_be_base()) {
        url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<Args>(args)...);
        return url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, url_parser::host_state) == url_result::Ok;
    }
    return false;
}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline bool url::hostname(Args&&... args) {
    if (!cannot_be_base()) {
        url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<Args>(args)...);
        return url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, url_parser::hostname_state) == url_result::Ok;
    }
    return false;
}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline bool url::port(Args&&... args) {
    if (canHaveUsernamePasswordPort()) {
        url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<Args>(args)...);
        const auto* first = inp.begin();
        const auto* last = inp.end();

        if (first == last) {
            urls.clear_part(url::PORT);
            return true;
        } else {
            return url_parser::url_parse(urls, first, last, nullptr, url_parser::port_state) == url_result::Ok;
        }
    }
    return false;
}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline bool url::pathname(Args&&... args) {
    if (!cannot_be_base()) {
        url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<Args>(args)...);
        return url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, url_parser::path_start_state) == url_result::Ok;
    }
    return false;
}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline bool url::search(Args&&... args) {
    bool res;
    {
        url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<Args>(args)...);
        const auto* first = inp.begin();
        const auto* last = inp.end();

        if (first == last) {
            urls.clear_part(url::QUERY);
            // empty context object's query object's list
            clear_search_params();
            return true;
        }
        if (*first == '?') ++first;
        res = url_parser::url_parse(urls, first, last, nullptr, url_parser::query_state) == url_result::Ok;
    }
    // set context object's query object's list to the result of parsing input
    parse_search_params();
    return res;
}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline bool url::hash(Args&&... args) {
    url_setter urls(*this);

    const auto inp = make_str_arg(std::forward<Args>(args)...);
    const auto* first = inp.begin();
    const auto* last = inp.end();

    if (first == last) {
        urls.clear_part(url::FRAGMENT);
        return true;
    }
    if (*first == '#') ++first;
    return url_parser::url_parse(urls, first, last, nullptr, url_parser::fragment_state) == url_result::Ok;
}


// https://url.spec.whatwg.org/#concept-basic-url-parser
template <typename CharT>
inline url_result url_parser::url_parse(url_serializer& urls, const CharT* first, const CharT* last, const url* base, State state_override)
{
    using UCharT = typename std::make_unsigned<CharT>::type;

    // remove all ASCII tab or newline from URL
    simple_buffer<CharT> buff_no_ws;
    detail::do_remove_whitespace(first, last, buff_no_ws);
    //TODO-WARN: validation error if removed

    // reserve size (TODO: But what if `base` is used?)
    auto length = std::distance(first, last);
    urls.reserve(length + 32);

#if WHATWG_URL_USE_ENCODING
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
            //TODO-ERR: validation error
            return url_result::InvalidSchemeCharacter;
        }
    }

    if (state == scheme_state) {
        bool is_scheme = false;
        state = no_scheme_state;
        auto it_colon = std::find(pointer, last, ':');
        // Deviation from URL stdandart's [ 2. ... if c is ":", run ... ] to
        // [ 2. ... if c is ":", or EOF and state override is given, run ... ]
        // This lets protocol setter to pass input without adding ':' to the end.
        // Similiar deviation exists in nodejs, see:
        // https://github.com/nodejs/node/pull/11917#pullrequestreview-28061847
        if (it_colon < last || state_override) {
            is_scheme = true;
            // start of scheme
            std::string& str_scheme = urls.start_scheme();
            // first char
            str_scheme.push_back(detail::kSchemeCanonical[static_cast<UCharT>(*pointer)]);
            // other chars
            for (auto it = pointer + 1; it < it_colon; ++it) {
                UCharT ch = static_cast<UCharT>(*it);
                char c = (ch < 0x80) ? detail::kSchemeCanonical[ch] : 0;
                if (c) {
                    str_scheme.push_back(c);
                } else {
                    is_scheme = false;
                    break;
                }
            }
            if (is_scheme) {
                // kas toliau
                if (state_override) {
                    const url::scheme_info* scheme_inf = url::get_scheme_info(str_scheme);
                    const bool is_special_old = urls.is_special_scheme();
                    const bool is_special_new = scheme_inf && scheme_inf->is_special;
                    if (is_special_old != is_special_new)
                        return url_result::False;
                    // new URL("http://u:p@host:88/).protocol("file:");
                    if (scheme_inf && scheme_inf->is_file && (urls.has_credentials() || !urls.is_null(url::PORT)))
                        return url_result::False;
                    // new URL("file:///path).protocol("http:");
                    if (urls.is_file_scheme() && urls.is_empty(url::HOST))
                        return url_result::False;
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
                    return url_result::Ok;
                }
                urls.save_scheme();
                pointer = it_colon + 1; // skip ':'
                if (urls.is_file_scheme()) {
                    // TODO-WARN: if remaining does not start with "//", validation error.
                    state = file_state;
                } else {
                    if (urls.is_special_scheme()) {
                        if (base && urls.get_part_view(url::SCHEME) == base->get_part_view(url::SCHEME)) {
                            // NOTE: This means that base's cannot-be-a-base-URL flag is unset
                            state = special_relative_or_authority_state;
                        } else {
                            state = special_authority_slashes_state;
                        }
                    } else if (pointer < last && *pointer == '/') {
                        state = path_or_authority_state;
                        ++pointer;
                    } else {
                        urls.set_cannot_be_base();
                        // append an empty string to url's path
                        // urls.append_to_path();
                        urls.start_path_string(); // ne start_path_segment()
                        urls.save_path_string();
                        state = cannot_be_base_URL_path_state;
                    }
                }
            } else {
                // remove invalid scheme
                urls.clear_scheme();
            }
        }
        if (state_override && !is_scheme) {
            //TODO-ERR: validation error
            return url_result::InvalidSchemeCharacter;
        }
    }

    if (state == no_scheme_state) {
        if (base) {
            if (base->cannot_be_base()) {
                if (pointer < last && *pointer == '#') {
                    // IMPORTANT: set_cannot_be_base() must be called before append_parts(..),
                    // because the cannot_be_base flag can be used when serializing in
                    // append_parts(..)
                    urls.set_cannot_be_base();
                    urls.set_scheme(*base);
                    urls.append_parts(*base, url::PATH, url::QUERY);
                    //TODO: url's fragment to the empty string
                    state = fragment_state;
                    ++pointer;
                } else {
                    // TODO-ERR: 1. validation error
                    return url_result::RelativeUrlWithCannotBeABase;
                }
            } else {
                state = base->is_file_scheme() ? file_state : relative_state;
            }
        } else {
            //TODO-ERR: 1. validation error
            return url_result::RelativeUrlWithoutBase;
        }
    }

    if (state == special_relative_or_authority_state) {
        if (pointer + 1 < last && pointer[0] == '/' && pointer[1] == '/') {
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
            return url_result::Ok; // EOF
        } else {
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
            default:
                // Set url's username to base's username, url's password to base's password, url's host to base's host,
                // url's port to base's port, url's path to base's path, and then remove url's path's last entry, if any
                urls.append_parts(*base, url::USERNAME, url::PATH, &url::get_path_rem_last);
                state = path_state;
                --pointer;
            }
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
        default:
            // set url's username to base's username, url's password to base's password, url's host to base's host,
            // url's port to base's port
            urls.append_parts(*base, url::USERNAME, url::PORT);
            state = path_state;
        }
    }

    if (state == special_authority_slashes_state) {
        if (pointer + 1 < last && pointer[0] == '/' && pointer[1] == '/') {
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

    // TODO?: credentials serialization do after host parsing,
    //       because if host null ==> then no credentials serialization
    if (state == authority_state) {
        // TODO: saugoti end_of_authority ir naudoti kituose state
        auto end_of_authority = urls.is_special_scheme() ?
            std::find_if(pointer, last, detail::is_special_authority_end_char<CharT>) :
            std::find_if(pointer, last, detail::is_authority_end_char<CharT>);

        auto it_eta = detail::find_last(pointer, end_of_authority, CharT('@'));
        if (it_eta != end_of_authority) {
            if (std::distance(it_eta, end_of_authority) == 1) {
                // disallow empty host, example: "http://u:p@/"
                return url_result::EmptyHost; // TODO-ERR: 2.1. validation error, failure
            }
            //TODO-WARN: validation error
            auto it_colon = std::find(pointer, it_eta, ':');
            // url includes credentials?
            const bool not_empty_password = std::distance(it_colon, it_eta) > 1;
            if (not_empty_password || std::distance(pointer, it_colon) > 0 /*not empty username*/) {
                // username
                std::string& str_username = urls.start_part(url::USERNAME);
                detail::AppendStringOfType(pointer, it_colon, detail::CHAR_USERINFO, str_username); // UTF-8 percent encode, @ -> %40
                urls.save_part();
                // password
                if (not_empty_password) {
                    std::string& str_password = urls.start_part(url::PASSWORD);
                    detail::AppendStringOfType(it_colon + 1, it_eta, detail::CHAR_USERINFO, str_password); // UTF-8 percent encode, @ -> %40
                    urls.save_part();
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
            auto end_of_authority = urls.is_special_scheme() ?
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
                    // TODO-ERR: validation error
                    return url_result::EmptyHost;
                } else if (state_override && (urls.has_credentials() || !urls.is_null(url::PORT))) {
                    // TODO-WARN: validation error (empty host)
                    return url_result::False;
                }
            }

            // parse and set host:
            const url_result res = parse_host(urls, pointer, it_host_end);
            if (res != url_result::Ok)
                return res; // TODO-ERR: failure

            if (is_port) {
                pointer = it_host_end + 1; // skip ':'
                state = port_state;
                if (state_override == hostname_state)
                    return url_result::Ok;
            } else {
                pointer = it_host_end;
                state = path_start_state;
                if (state_override)
                    return url_result::Ok;
            }
        }
    }

    if (state == port_state) {
        auto end_of_digits = std::find_if_not(pointer, last, detail::is_ascii_digit<CharT>);

        const bool is_end_of_authority =
            end_of_digits == last || // EOF
            detail::is_authority_end_char(end_of_digits[0]) ||
            (end_of_digits[0] == '\\' && urls.is_special_scheme());

        if (is_end_of_authority || state_override) {
            if (pointer < end_of_digits) {
                // is port
                int port = 0;
                for (auto it = pointer; it < end_of_digits; ++it) {
                    port = port * 10 + (*it - '0');
                    if (port > 0xFFFF)
                        return url_result::InvalidPort; // TODO-ERR: (2-1-2) validation error, failure
                }
                // set port if not default
                if (urls.scheme_inf() == nullptr || urls.scheme_inf()->default_port != port) {
                    unsigned_to_str(port, urls.start_part(url::PORT), 10);
                    urls.save_part();
                    urls.set_flag(url::PORT_FLAG);
                } else {
                    // (2-1-3) Set url's port to null
                    urls.clear_part(url::PORT);
                }
            }
            if (state_override)
                return url_result::Ok; // (2-2)
            state = path_start_state;
            pointer = end_of_digits;
        } else {
            // TODO-ERR: (3) validation error, failure (contains non-digit)
            return url_result::InvalidPort;
        }
    }

    if (state == file_state) {
        if (!urls.is_file_scheme())
            urls.set_scheme(url::str_view_type{ "file", 4 });
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
                    return url_result::Ok; // EOF
                } else {
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
                        if (!detail::starts_with_Windows_drive(pointer, last)) {
                            // set url's host to base's host, url's path to base's path, and then shorten url's path
                            urls.append_parts(*base, url::HOST, url::PATH, &url::get_shorten_path);
                            // Note: This is a (platform-independent) Windows drive letter quirk.
                        }
                        //else // TODO-WARN: validation error
                        state = path_state;
                    }
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
            if (base && base->is_file_scheme() &&
                !detail::starts_with_Windows_drive(pointer, last)) {
                url::str_view_type base_path = base->get_path_first_string(2);
                // if base's path[0] is a normalized Windows drive letter
                if (base_path.length() == 2 &&
                    detail::is_normalized_Windows_drive(base_path[0], base_path[1])) {
                    // append base's path[0] to url's path
                    std::string& str_path = urls.start_path_segment();
                    str_path.append(base_path.data(), 2); // "C:"
                    urls.save_path_segment();
                    // Note: This is a (platform - independent) Windows drive letter quirk.
                    // Both url's and base's host are null under these conditions and therefore not copied.
                } else {
                    // set url's host to base's host
                    urls.append_parts(*base, url::HOST, url::HOST);
                }
            }
            state = path_state;
        }
    }

    if (state == file_host_state) {
        auto end_of_authority = std::find_if(pointer, last, detail::is_special_authority_end_char<CharT>);

        if (pointer == end_of_authority) {
            // buffer is the empty string
            // set empty host
            urls.start_part(url::HOST);
            urls.save_part();
            urls.set_host_type(HostType::Empty);
            // if state override is given, then return
            if (state_override)
                return url_result::Ok;
            state = path_start_state;
        } else if (!state_override && pointer + 2 == end_of_authority &&
            detail::is_Windows_drive(pointer[0], pointer[1])) {
            // buffer is a Windows drive letter
            // TODO-WARN: validation error
            state = path_state;
            // Note: This is a (platform - independent) Windows drive letter quirk.
            // buffer is not reset here and instead used in the path state.
            // TODO: buffer is not reset here and instead used in the path state
        } else {
            // parse and set host:
            const url_result res = parse_host(urls, pointer, end_of_authority);
            if (res != url_result::Ok)
                return res; // TODO-ERR: failure
            // if host is "localhost", then set host to the empty string
            if (urls.get_part_view(url::HOST) == url::str_view_type{ "localhost", 9 }) {
                // set empty host
                urls.empty_host();
            }
            // if state override is given, then return
            if (state_override)
                return url_result::Ok;
            pointer = end_of_authority;
            state = path_start_state;
        }
    }

    if (state == path_start_state) {
        if (urls.is_special_scheme()) {
            if (pointer != last) {
                const CharT ch = *pointer;
                if (ch == '/') ++pointer;
                else if (ch == '\\') {
                    // TODO-WARN: validation error
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
            urls.commit_path(); // path is empty
            return url_result::Ok;
        }
    }

    if (state == path_state) {
        auto end_of_path = state_override ? last :
            std::find_if(pointer, last, [](CharT c) { return c == '?' || c == '#'; });

        parse_path(urls, pointer, end_of_path);
        pointer = end_of_path;

        // trim leading slashes of file URL path
        if (urls.is_file_scheme()) {
            urls.remove_leading_path_slashes();
            // if (urls.remove_leading_path_slashes() > 0) // TODO-WARN: validation error
        }

        // the end of path parse
        urls.commit_path();

        if (pointer == last) {
            return url_result::Ok; // EOF
        } else {
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
    }

    if (state == cannot_be_base_URL_path_state) {
        auto end_of_path =
            std::find_if(pointer, last, [](CharT c) { return c == '?' || c == '#'; });

        // UTF-8 percent encode using the C0 control percent-encode set,
        // and append the result to url's path[0]
        std::string& str_path = urls.start_path_string(); // ne start_path_segment()
        do_simple_path(pointer, end_of_path, str_path);
        urls.save_path_string();
        pointer = end_of_path;

        if (pointer == last) {
            return url_result::Ok; // EOF
        } else {
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
    }

    if (state == query_state) {
        auto end_of_query = state_override ? last : std::find(pointer, last, '#');

        // TODO-WARN:
        //for (auto it = pointer; it < end_of_query; ++it) {
        //  UCharT c = static_cast<UCharT>(*it);
        //  // 1. If c is not a URL code point and not "%", validation error.
        //  // 2. If c is "%" and remaining does not start with two ASCII hex digits, validation error.
        //}

#if WHATWG_URL_USE_ENCODING
        // scheme_inf_ == nullptr, if unknown scheme
        if (!urls.scheme_inf() || !urls.scheme_inf()->is_special || urls.scheme_inf()->is_ws)
            encoding = "UTF-8";
#endif

        // Set buffer to the result of encoding buffer using encoding
        // percent encode: (c < 0x21 || c > 0x7E || c == 0x22 || c == 0x23 || c == 0x3C || c == 0x3E)
        // TODO: now supports UTF-8 encoding only, maybe later to add other encodings support
        std::string& str_query = urls.start_part(url::QUERY);
        //detail::AppendStringOfType(pointer, end_of_query, detail::CHAR_QUERY, str_query);
        const bool is_special = urls.is_special_scheme();
        while (pointer != end_of_query) {
            // UTF-8 percent encode c using the fragment percent-encode set
            // and ignore '\0'
            UCharT uch = static_cast<UCharT>(*pointer);
            if (uch >= 0x80) {
                // invalid utf-8/16/32 sequences will be replaced with kUnicodeReplacementCharacter
                detail::AppendUTF8EscapedChar(pointer, end_of_query, str_query);
            } else {
                // Just append the 7-bit character, possibly escaping it.
                unsigned char uc = static_cast<unsigned char>(uch);
                if (!isCharInSet(uc, detail::CHAR_QUERY) || (uc == 0x27 && is_special))
                    detail::AppendEscapedChar(uch, str_query);
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
            return url_result::Ok; // EOF
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
            UCharT uch = static_cast<UCharT>(*pointer);
            if (uch >= 0x80) {
                // invalid utf-8/16/32 sequences will be replaced with kUnicodeReplacementCharacter
                detail::AppendUTF8EscapedChar(pointer, last, str_frag);
            } else {
                // Just append the 7-bit character, possibly escaping it.
                unsigned char uc = static_cast<unsigned char>(uch);
                if (isCharInSet(uc, detail::CHAR_FRAGMENT)) {
                    str_frag.push_back(uc);
                } else {
                    // other characters are escaped
                    detail::AppendEscapedChar(uch, str_frag);
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

    return url_result::Ok;
}

// internal functions

template <typename CharT>
inline url_result url_parser::parse_host(url_serializer& urls, const CharT* first, const CharT* last) {
    return host_parser::parse_host(first, last, !urls.is_special_scheme(), urls);
}

template <typename CharT>
inline void url_parser::parse_path(url_serializer& urls, const CharT* first, const CharT* last) {
    // path state; includes:
    // 1. [ (/,\) - 1, 2, 3, 4 - [ 1 (if first segment), 2 ] ]
    // 2. [ 1 ... 4 ]
    static const auto escaped_dot = [](const CharT* const pointer) -> bool {
        return pointer[0] == '%' && pointer[1] == '2' &&
            (pointer[2] == 'e' || pointer[2] == 'E');
    };
    const auto double_dot = [](const CharT* const pointer, const std::size_t len) -> bool {
        if (len == 2) {
            return (pointer[0] == '.' && pointer[1] == '.');
        } else if (len == 4) {
            // ".%2e" or "%2e."
            return (pointer[0] == '.' && escaped_dot(pointer + 1)) ||
                (escaped_dot(pointer) && pointer[3] == '.');
        } else if (len == 6) {
            // "%2e%2e"
            return escaped_dot(pointer) && escaped_dot(pointer + 3);
        } else
            return false;
    };
    const auto single_dot = [](const CharT* const pointer, const std::size_t len) -> bool {
        if (len == 1) {
            return pointer[0] == '.';
        } else if (len == 3) {
            return escaped_dot(pointer); // "%2e"
        } else
            return false;
    };

    // parse path's segments
    auto pointer = first;
    while (true) {
        auto end_of_segment = urls.is_special_scheme()
            ? std::find_if(pointer, last, detail::is_slash<CharT>)
            : std::find(pointer, last, '/');

        // end_of_segment >= pointer
        const std::size_t len = end_of_segment - pointer;
        bool is_last = end_of_segment == last;
        // TODO-WARN: 1. If url is special and c is "\", validation error.

        if (double_dot(pointer, len)) {
            urls.shorten_path();
            if (is_last) urls.append_to_path();
        } else if (single_dot(pointer, len)) {
            if (is_last) urls.append_to_path();
        } else {
            if (len == 2 &&
                urls.is_file_scheme() &&
                urls.is_empty_path() &&
                detail::is_Windows_drive(pointer[0], pointer[1]))
            {
                if (!urls.is_empty(url::HOST)) {
                    // 1. If url's host is not the empty string, validation error
                    // TODO-WARN: validation error
                    // 2. Set url's host to the empty string
                    urls.empty_host();
                }
                // and replace the second code point in buffer with ":"
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
        UCharT uch = static_cast<UCharT>(*pointer);
        if (uch >= 0x80) {
            // invalid utf-8/16/32 sequences will be replaced with 0xfffd
            success &= detail::AppendUTF8EscapedChar(pointer, last, output);
        } else {
            // Just append the 7-bit character, possibly escaping it.
            unsigned char uc = static_cast<unsigned char>(uch);
            if (!isCharInSet(uc, detail::CHAR_PATH))
                detail::AppendEscapedChar(uc, output);
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

    // 3. of "cannot-be-a-base-URL path state"
    // TODO-WARN: 3. [ 1 ... 2 ] validation error.
    //  1. If c is not EOF code point, not a URL code point, and not "%", validation error.
    //  2. If c is "%" and remaining does not start with two ASCII hex digits, validation error.

    bool success = true;
    while (pointer < last) {
        // UTF-8 percent encode c using the C0 control percent-encode set (U+0000 ... U+001F and >U+007E)
        UCharT uch = static_cast<UCharT>(*pointer);
        if (uch >= 0x7f) {
            // invalid utf-8/16/32 sequences will be replaced with 0xfffd
            success &= detail::AppendUTF8EscapedChar(pointer, last, output);
        } else {
            // Just append the 7-bit character, escaping C0 control chars:
            if (uch <= 0x1f)
                detail::AppendEscapedChar(uch, output);
            else
                output.push_back(static_cast<unsigned char>(uch));
            ++pointer;
        }
    }
    return success;
}

// path util

inline url::str_view_type url::get_path_first_string(std::size_t len) const {
    str_view_type pathv = get_part_view(PATH);
    if (pathv.length() == 0 || cannot_be_base())
        return pathv;
    // skip '/'
    pathv.remove_prefix(1);
    if (pathv.length() == len || (pathv.length() > len && pathv[len] == '/')) {
        return str_view_type(pathv.data(), len);
    }
    return str_view_type(pathv.data(), 0);
}

// path shortening

inline bool url::get_path_rem_last(std::size_t& path_end, std::size_t& path_segment_count) const {
    if (path_segment_count_ > 0) {
        // Remove path's last item
        const char* first = norm_url_.data() + part_end_[url::PATH-1];
        const char* last = norm_url_.data() + part_end_[url::PATH];
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
    if (path_segment_count_ == 0)
        return false;
    if (is_file_scheme() && path_segment_count_ == 1) {
        str_view_type path1 = get_path_first_string(2);
        if (path1.length() == 2 &&
            detail::is_normalized_Windows_drive(path1[0], path1[1]))
            return false;
    }
    // Remove path's last item
    return get_path_rem_last(path_end, path_segment_count);
}

// url_serializer class

inline void url_serializer::shorten_path() {
    assert(last_pt_ <= url::PATH);
    if (url_.get_shorten_path(url_.part_end_[url::PATH], url_.path_segment_count_))
        url_.norm_url_.resize(url_.part_end_[url::PATH]);
}

static inline std::size_t count_leading_path_slashes(const char* first, const char* last) {
    return std::distance(first,
        std::find_if_not(first, last, [](char c){ return c == '/'; }));
}

inline std::size_t url_serializer::remove_leading_path_slashes() {
    assert(last_pt_ == url::PATH);
    std::size_t count = count_leading_path_slashes(
        url_.norm_url_.data() + url_.part_end_[url::PATH-1],
        url_.norm_url_.data() + url_.part_end_[url::PATH]);
    if (count > 1) {
        count -= 1;
        url_.norm_url_.erase(url_.part_end_[url::PATH-1], count);
        url_.part_end_[url::PATH] -= count;
        url_.path_segment_count_ -= count;
        return count;
    }
    return 0;
}

inline url_serializer::~url_serializer() {
    switch (last_pt_) {
    case url::SCHEME:
        // if url's host is null and url's scheme is "file"
        if (url_.is_file_scheme())
            url_.norm_url_.append("//");
        break;
    default: break;
    }
}

// set/clear scheme

inline std::string& url_serializer::start_scheme() {
    url_.norm_url_.resize(0);
    return url_.norm_url_;
}

inline void url_serializer::save_scheme() {
    set_scheme(url_.norm_url_.length());
    url_.norm_url_.push_back(':');
}

inline void url_serializer::clear_scheme() {
    assert(last_pt_ == url::SCHEME);
    url_.clear_scheme();
}

// set url's part

inline void url_serializer::fill_parts_offset(url::PartType t1, url::PartType t2, std::size_t offset) {
    for (int ind = t1; ind < t2; ++ind)
        url_.part_end_[ind] = offset;
}

inline std::string& url_serializer::start_part(url::PartType new_pt) {
    // offsets of empty parts (until new_pt) are also filled
    url::PartType fill_start_pt = static_cast<url::PartType>(static_cast<int>(last_pt_)+1);
    switch (last_pt_) {
    case url::SCHEME:
        // if host is non-null or scheme is "file"
        if (new_pt <= url::HOST || url_.is_file_scheme())
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
    case url::PASSWORD:
        if (new_pt == url::HOST) {
            url_.norm_url_ += '@';
            break;
        }
        // NOT REACHABLE (TODO: throw?)
        break;
    case url::HOST:
        if (new_pt == url::PORT) {
            url_.norm_url_ += ':';
            break;
        }
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
    case url::QUERY:
        url_.norm_url_ += '?';
        break;
    case url::FRAGMENT:
        url_.norm_url_ += '#';
        break;
    default: break;
    }

    assert(last_pt_ < new_pt);
    // value to url_.part_end_[new_pt] will be assigned in the save_part()
    last_pt_ = new_pt;
    return url_.norm_url_;
}

inline void url_serializer::save_part() {
    url_.part_end_[last_pt_] = url_.norm_url_.length();
}

// The append_empty_to_path() is called from these places:
// 1) scheme_state -> [2.9.] -> cannot_be_base_URL_path_state
// 2) path_state -> [1.2. ".." ]
// 3) path_state -> [1.3. "." ]
inline void url_serializer::append_to_path() { // append_empty_to_path();
    start_path_segment();
    save_path_segment();
}

inline std::string& url_serializer::start_path_segment() {
    // lipdom prie esamo kelio: / seg1 / seg2 / ... / segN
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
    // https://url.spec.whatwg.org/#url-serializing (5.1.)
    if (is_null(url::HOST) && url_.path_segment_count_ > 1) {
        const auto pathname = get_part_view(url::PATH);
        if (pathname.length() > 1 && pathname[0] == '/' && pathname[1] == '/') {
            if (is_empty(url::PATH_PREFIX)) {
                replace_part(url::PATH_PREFIX, "/.", 2);
            }
        }
    }
}

inline std::string& url_serializer::start_path_string() {
    //return start_part(url::PATH);
    if (last_pt_ != url::PATH)
        return start_part(url::PATH);
    return url_.norm_url_;
}

inline void url_serializer::save_path_string() {
    assert(url_.path_segment_count_ <= 1);
    save_part();
    url_.path_segment_count_ = 1;
}


inline void url_serializer::clear_host() {
    //NOT USED
    empty_host();
    url_.flags_ &= ~(url::HOST_FLAG | url::HOST_TYPE_MASK); // set to null
}

inline void url_serializer::empty_host() {
    assert(last_pt_ >= url::HOST);
    const std::size_t host_end = url_.part_end_[url::HOST_START];
    if (last_pt_ == url::HOST) {
        url_.part_end_[url::HOST] = host_end;
        url_.norm_url_.resize(host_end);
    } else if (last_pt_ > url::HOST) {
        const std::size_t diff = url_.part_end_[url::HOST] - host_end;
        if (diff) {
            for (int pt = url::HOST; pt <= last_pt_; ++pt)
                url_.part_end_[pt] -= diff;
            url_.norm_url_.erase(host_end, diff);
        }
    }
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
    url::PartType ifirst;
    if (t1 <= url::HOST) {
        // authority, host
        if (!src.is_null(url::HOST)) {
            if (t1 == url::USERNAME && src.has_credentials())
                ifirst = url::USERNAME;
            else
                ifirst = url::HOST;
        } else {
            ifirst = url::PATH_PREFIX;
        }
    } else {
        // t1 == PATH
        ifirst = t1;
    }

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
            const char* first = src.norm_url_.data() + offset;
            const char* last = src.norm_url_.data() + lastp_end;
            // dest
            const std::ptrdiff_t delta = checked_diff<std::ptrdiff_t>(norm_url.length(), offset);
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

    // copy host_type & not null flags
    unsigned mask = url::HOST_TYPE_MASK;
    for (int ind = t1; ind <= t2; ++ind) {
        mask |= (1u << ind);
    }
    url_.flags_ = (url_.flags_ & ~mask) | (src.flags_ & mask);
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
    const std::ptrdiff_t diff = checked_diff<std::ptrdiff_t>(len, l);
    if (diff) {
        for (auto it = std::begin(url_.part_end_) + last_pt; it != std::end(url_.part_end_); ++it) {
            if (*it == 0) break;
            // perform arithmetics using signed type ptrdiff_t, because diff can be negative
            *it = static_cast<std::ptrdiff_t>(*it) + diff;
        }
    }
}


// url_setter class

inline url_setter::~url_setter() {
    //todo: ~url_serializer() must do nothing
    last_pt_ = url::FRAGMENT;
}

//???
inline void url_setter::reserve(std::size_t new_cap) {
    strp_.reserve(new_cap);
}

// set/clear scheme

inline std::string& url_setter::start_scheme() {
    return strp_;
}

inline void url_setter::save_scheme() {
    replace_part(url::SCHEME, strp_.data(), strp_.length());
    set_scheme(strp_.length());
}

inline void url_setter::clear_scheme() {
    //?? assert(last_pt_ == url::SCHEME);
    strp_.clear();
}

// set/clear/empty url's part

inline std::string& url_setter::start_part(url::PartType new_pt) {
    assert(new_pt > url::SCHEME);
    curr_pt_ = new_pt;
    if (url_.part_end_[new_pt]) {
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
        case url::FRAGMENT:
            strp_ = '#';
            break;
        default:
            strp_.clear();
            break;
        }
        return strp_;
    } else {
        use_strp_ = false;
        last_pt_ = find_last_part(new_pt);
        return url_serializer::start_part(new_pt);
    }
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
    // https://url.spec.whatwg.org/#url-serializing (5.1.)
    const bool no_prefix = is_empty(url::PATH_PREFIX);
    str_view_type new_prefix;
    if (is_null(url::HOST) && url_.path_segment_count_ > 1) {
        const auto pathname = get_part_view(url::PATH);
        if (pathname.length() > 1 && pathname[0] == '/' && pathname[1] == '/')
            new_prefix = { "/.", 2 };
    }
    if (no_prefix != new_prefix.empty())
        replace_part(url::PATH_PREFIX, new_prefix.data(), new_prefix.length());
}

// https://url.spec.whatwg.org/#shorten-a-urls-path

inline void url_setter::shorten_path() {
    if (path_seg_end_.size() == 1) {
        if (is_file_scheme() && strp_.length() == 3 &&
            detail::is_normalized_Windows_drive(strp_[1], strp_[2]))
            return;
        path_seg_end_.pop_back();
        strp_.clear();
    } else if (path_seg_end_.size() >= 2) {
        path_seg_end_.pop_back();
        strp_.resize(path_seg_end_.back());
    }
}

inline std::size_t url_setter::remove_leading_path_slashes() {
    std::size_t count = count_leading_path_slashes(strp_.data(), strp_.data() + strp_.length());
    if (count > 1) {
        count -= 1;
        strp_.erase(0, count);
        path_seg_end_.erase(path_seg_end_.begin(), std::next(path_seg_end_.begin(), count));
        return count;
    }
    return 0;
}

inline bool url_setter::is_empty_path() const {
    return path_seg_end_.empty();
}

inline url::PartType url_setter::find_last_part(url::PartType pt) const {
    for (int ind = pt; ind > 0; --ind)
        if (url_.part_end_[ind])
            return static_cast<url::PartType>(ind);
    return url::SCHEME;
}

#if 0
inline void url_setter::insert_part(url::PartType new_pt, const char* str, std::size_t len) {
    assert(new_pt > url::SCHEME);
    if (url_.part_end_[new_pt]) {
        replace_part(new_pt, str, len);
    } else {
        last_pt_ = find_last_part(new_pt);
        url_serializer::start_part(new_pt).append(str, len);
        url_serializer::save_part();
    }
}
#endif


} // namespace whatwg

#endif // WHATWG_URL_H
