// Copyright 2016 Rimas Misevičius
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// URL Standard
// https://url.spec.whatwg.org/ [18 January 2017]
// https://url.spec.whatwg.org/commit-snapshots/960f6078a96e688c59ea3cfc3853a764d46179cf/
//
// Infra Standard - fundamental concepts upon which standards are built
// https://infra.spec.whatwg.org/ [13 January 2017]
// https://infra.spec.whatwg.org/commit-snapshots/f817d690ee9f1a7556805d4796a6ebbfe6eb127f/

#ifndef WHATWG_URL_H
#define WHATWG_URL_H

#include "buffer.h"
#include "url_canon.h"
#include "url_idna.h"
#include "url_ip.h"
#include "url_util.h"
#include <algorithm>
#include <cassert>
#include <string>
#include <type_traits>


template<typename CharT, typename Traits = std::char_traits<CharT>>
class str_view {
public:
    str_view(const CharT* ptr, std::size_t len) : ptr_(ptr), len_(len) {}

    const CharT* data() const { return ptr_; }
    std::size_t length() const { return len_; }
    std::size_t size() const { return len_; }

    CharT operator[](std::size_t ind) const {
        return ptr_[ind];
    }

    int compare(str_view x) const {
        const int cmp = Traits::compare(ptr_, x.ptr_, std::min(len_, x.len_));
        return cmp != 0 ? cmp : (len_ == x.len_ ? 0 : len_ < x.len_ ? -1 : 1);
    }

    bool equal(str_view x) const {
        return len_ == x.len_ && Traits::compare(ptr_, x.ptr_, len_) == 0;
    }

    void append_to(std::basic_string<CharT>& str) const {
        str.append(ptr_, len_);
    }

private:
    const CharT* ptr_;
    std::size_t len_;
};

namespace whatwg {

namespace detail {

struct url_part {
    url_part() : offset(0), len(0) {}
    url_part(std::size_t b, std::size_t l) : offset(b), len(l) {}

    template <class InputIt>
    url_part(InputIt begin, InputIt first, InputIt last)
        : offset(first - begin), len(last - first) {}

    static url_part from_range(std::size_t b, std::size_t e) {
        return url_part(b, e - b);
    }

    bool empty() const { return len == 0; }

    std::size_t offset; // Byte offset in the string of this component.
    std::size_t len;    // Will be -1 if the component is unspecified.
};


struct scheme_info {
    str_view<char> scheme;
    int default_port;           // -1 if none
    unsigned is_special: 1;     // "ftp", "file", "gopher", "http", "https", "ws", "wss"
    unsigned is_local : 1;      // "about", "blob", "data", "filesystem"
    unsigned is_http : 1;       // "http", "https"
    unsigned is_network : 1;    // "ftp" or an HTTP(S) scheme
    unsigned is_fetch : 1;      // "about", "blob", "data", "file", "filesystem", or a network scheme
    // other
    unsigned is_file : 1;       // "file"
    unsigned is_ws : 1;         // "ws", "wss"

    // for search operations
    operator const str_view<char>&() const { return scheme; }
};

template <typename CharT>
bool str_equal(const CharT* first, const CharT* last, const char* strz) {
    for (const CharT* it = first; it < last; it++) {
        if (*strz == 0 || *it != *strz) return false;
        strz++;
    }
    return *strz == 0;
}

template <typename CharT>
bool part_equal(const CharT* begin, const url_part& part, const char* strz) {
    return str_equal(begin + part.offset, begin + part.offset + part.len, strz);
}

// Lowercase the ASCII character
template <typename CharT>
inline CharT AsciiToLower(CharT c) {
    return (c <= 'Z' && c >= 'A') ? (c | 0x20) : c;
}

// strz must be in lower case
template <typename CharT>
inline bool AsciiEqualNoCase(const CharT* first, const CharT* last, const char* strz) {
    for (const CharT* it = first; it < last; it++) {
        if (*strz == 0 || AsciiToLower(*it) != *strz) return false;
        strz++;
    }
    return *strz == 0;
}

extern const char kSchemeCanonical[0x80];

extern const scheme_info* get_scheme_info(const str_view<char> src);
inline const scheme_info* get_scheme_info(const char* name, std::size_t len) {
    return get_scheme_info(str_view<char>(name, len));
}


} // namespace detail

// URL class

class url {
public:
    enum PartType {
        SCHEME = 0,
        USERNAME,
        PASSWORD,
        HOST,
        PORT,
        PATH,
        QUERY,
        FRAGMENT,
        PART_COUNT
    };

    /**
    * \brief Default constructor.
    * Constructs empty URL
    */
    url()
        : scheme_inf_(nullptr)
        , flags_(INITIAL_FLAGS)
        , path_segment_count_(0)
    {}

    void clear();

    template <typename CharT>
    bool parse(const CharT* first, const CharT* last, const url* base);
    // wchar_t
    bool parse(const wchar_t* first, const wchar_t* last, const url* base) {
        typedef std::conditional<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>::type CharT;
        return parse(reinterpret_cast<const CharT*>(first), reinterpret_cast<const CharT*>(last), base);
    }

    template <class BufferT>
    bool parse(const BufferT& str, const url* base) {
        return parse(str.data(), str.data() + str.size(), base);
    }

    template <typename CharT>
    bool parse(const CharT* strz, const url* base) {
        const CharT* last = strz;
        while (*last) last++;
        return parse(strz, last, base);
    }

    // get serialized URL
    const std::string& get_href() const {
        return norm_url_;
    }

    std::string get_protocol() const {
        return std::string(norm_url_.data() + part_[SCHEME].offset, part_[SCHEME].len ? part_[SCHEME].len + 1 : 0);
    }

    std::string get_host() const {
        if (is_null(HOST))
            return std::string("");
        if (is_null(PORT))
            return get_part(HOST);
        // "host:port"
        const size_t offset = is_empty(HOST) ? part_[PORT].offset - 1 : part_[HOST].offset;
        const size_t iend = part_[PORT].offset + part_[PORT].len;
        return std::string(norm_url_.data() + offset, iend - offset);
    }

    std::string get_pathname() const {
        // https://url.spec.whatwg.org/#dom-url-pathname
        // already serialized as needed
        return get_part(PATH);
    }

    std::string get_search() const {
        if (is_empty(QUERY))
            return std::string("");
        return std::string(norm_url_.data() + part_[QUERY].offset - 1, part_[QUERY].len + 1);
    }

    std::string get_hash() const {
        if (is_empty(FRAGMENT))
            return std::string("");
        return std::string(norm_url_.data() + part_[FRAGMENT].offset - 1, part_[FRAGMENT].len + 1);
    }

    std::string get_part(PartType t) const {
        return std::string(norm_url_.data() + part_[t].offset, part_[t].len);
    }

    str_view<char> get_part_view(PartType t) const {
        return str_view<char>(norm_url_.data() + part_[t].offset, part_[t].len);
    }

    bool is_empty(const PartType t) const {
        return part_[t].empty();
    }

    bool is_null(const PartType t) const {
        return !(flags_ & (1u << t));
    }

    bool is_special_scheme() const {
        return scheme_inf_ && scheme_inf_->is_special;
    }

    bool is_file_scheme() const {
        return scheme_inf_ && scheme_inf_->is_file;
    }

    // URL includes credentials?
    bool has_credentials() const {
        return !is_empty(USERNAME) || !is_empty(PASSWORD);
    }

protected:
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
        // initial flags (empty (but not null) parts)
        INITIAL_FLAGS = SCHEME_FLAG | USERNAME_FLAG | PASSWORD_FLAG | PATH_FLAG,
    };

    bool is_null(const UrlFlag flag) const {
        return !(flags_ & flag);
    }

    template <typename CharT>
    bool parse_host(const CharT* first, const CharT* last);

    template <typename CharT>
    ParseResult parse_ipv4(const CharT* first, const CharT* last);

    template <typename CharT>
    bool parse_ipv6(const CharT* first, const CharT* last);

    template <typename CharT>
    void parse_path(const CharT* first, const CharT* last);

    template <typename CharT>
    bool do_path_segment(const CharT* pointer, const CharT* last, std::string& output);

    template <typename CharT>
    bool do_simple_path(const CharT* pointer, const CharT* last, std::string& output);

    detail::url_part get_path_rem_last() const;
    detail::url_part get_shorten_path() const;
    bool get_path_rem_last(detail::url_part& part, unsigned& path_segment_count) const;
    bool get_shorten_path(detail::url_part& part, unsigned& path_segment_count) const;
    void shorten_path();
    void append_to_path();

    template <class StringT>
    void add_part(PartType t, const StringT str) {
        part_[t].offset = norm_url_.length();
        part_[t].len = str.length();
        norm_url_.append(str.data(), str.length());
    }
    template <class StringT>
    void add_part(PartType t, const StringT str, char c) {
        add_part(t, str);
        norm_url_ += c;
    }

    void set_part(PartType t, std::size_t b, std::size_t e) {
        part_[t] = detail::url_part::from_range(b, e);
    }

    void clear_host() {
        if (part_[HOST].len) {
            norm_url_.resize(part_[HOST].offset);
            part_[HOST].offset = 0;
            part_[HOST].len = 0;
        }
        flags_ &= ~HOST_FLAG; // set to null
    }

    void set_scheme(const url& src) {
        norm_url_.resize(0); // clear all
        add_part(SCHEME, src.get_part_view(SCHEME), ':');
        scheme_inf_ = src.scheme_inf_;
    }
    void set_scheme(const str_view<char> str) {
        norm_url_.resize(0); // clear all
        add_part(SCHEME, str, ':');
        scheme_inf_ = detail::get_scheme_info(str);
    }
    void set_scheme(std::size_t b, std::size_t e) {
        set_part(SCHEME, b, e);
        scheme_inf_ = detail::get_scheme_info(get_part_view(SCHEME));
    }

    void add_slash_slash() {
        // if norm_url_ is "scheme:" (without "//")
        if (part_[SCHEME].len && part_[SCHEME].len + 1 == norm_url_.length())
            norm_url_.append("//");
    }
    void if_file_add_slash_slash() {
        if (is_file_scheme()) add_slash_slash();
    }

    void append_parts(const url& src, PartType t1, PartType t2) {
        append_parts(src, t1, t2, detail::url_part(0,0));
    }
    void append_parts(const url& src, PartType t1, PartType t2, detail::url_part lastp);

    // flags
    void set_flag(const UrlFlag flag) {
        flags_ |= flag;
    }
    bool cannot_be_base() const {
        return !!(flags_ & CANNOT_BE_BASE_FLAG);
    }
    void cannot_be_base(const bool yes) {
        if (yes) flags_ |= CANNOT_BE_BASE_FLAG;
        else flags_ &= ~CANNOT_BE_BASE_FLAG;
    }

private:
    std::string norm_url_;
    detail::url_part part_[PART_COUNT];
    const detail::scheme_info* scheme_inf_;
    unsigned flags_;
    unsigned path_segment_count_;

    friend class url_serializer;
};


class url_serializer {
public:
    url_serializer(url& dest_url)
        : url_(dest_url)
        , last_pt_(url::SCHEME)
    {}

    ~url_serializer();

    // set data
    void set_scheme(const url& src) { url_.set_scheme(src); }
    void set_scheme(const str_view<char> str) { url_.set_scheme(str); }
    void set_scheme(std::size_t b, std::size_t e) { url_.set_scheme(b, e); }

    std::string& start_part(url::PartType t);
    void save_part();

    void clear_host();

    // path
    // TODO: append_to_path() --> append_empty_to_path()
    void append_to_path();
    std::string& start_path_segment();
    void save_path_segment();
    
    void shorten_path();

    typedef bool (url::*PathOpFn)(detail::url_part& part, unsigned& segment_count) const;
    void append_parts(const url& src, url::PartType t1, url::PartType t2, PathOpFn pathOpFn = nullptr);

    // flags
    void set_flag(const url::UrlFlag flag) { url_.set_flag(flag); }
    void cannot_be_base(const bool yes) { url_.cannot_be_base(yes); }

    // get info
    bool is_empty(const url::PartType t) const { return url_.is_empty(t); }
    bool is_empty_path() const { return url_.path_segment_count_ == 0; }
    bool is_null(const url::PartType t) const  { return url_.is_null(t); }
    bool is_special_scheme() const { return url_.is_special_scheme(); }
    bool is_file_scheme() const { return url_.is_file_scheme(); }

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
    url& url_;
    url::PartType last_pt_;
};


// Removable URL chars

template <typename CharT, typename UCharT = std::make_unsigned<CharT>::type>
auto to_unsigned(CharT ch) -> UCharT {
    return static_cast<std::make_unsigned<CharT>::type>(ch);
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

template <class InputIt>
inline void do_trim(InputIt& first, InputIt& last) {
    // remove leading C0 controls and space
    while (first < last && is_trim_char(*first))
        first++;
    // remove trailing C0 controls and space
    auto rit = std::reverse_iterator<InputIt>(last);
    auto rend = std::reverse_iterator<InputIt>(first);
    while (rit < rend && is_trim_char(*rit))
        rit++;
    last = rit.base();
}

// DoRemoveURLWhitespace
// https://cs.chromium.org/chromium/src/url/url_canon_etc.cc
template <typename CharT>
inline void do_remove_whitespace(const CharT*& first, const CharT*& last, simple_buffer<CharT>& buff) {
    // Fast verification that there’s nothing that needs removal. This is the 99%
    // case, so we want it to be fast and don't care about impacting the speed
    // when we do find whitespace.
    for (auto it = first; it < last; it++) {
        if (!is_removable_char(*it))
            continue;
        // copy non whitespace chars into the new buffer and return it
        buff.reserve(last - first);
        buff.append(first, it);
        for (; it < last; it++) {
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

template <typename CharT>
inline bool is_Windows_drive(CharT c1, CharT c2) {
    return is_ascii_alpha(c1) && (c2 == ':' || c2 == '|');
}

template <typename CharT>
inline bool is_normalized_Windows_drive(CharT c1, CharT c2) {
    return is_ascii_alpha(c1) && c2 == ':';
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

inline void url::clear() {
    norm_url_.resize(0);
    std::memset(part_, 0, sizeof(part_));
    scheme_inf_ = nullptr;
    flags_ = INITIAL_FLAGS;
    path_segment_count_ = 0;
}

// https://url.spec.whatwg.org/#concept-basic-url-parser
template <typename CharT>
inline bool url::parse(const CharT* first, const CharT* last, const url* base) {
    typedef std::make_unsigned<CharT>::type UCharT;

    // remove any leading and trailing C0 control or space:
    do_trim(first, last);
    //TODO-WARN: syntax violation if trimmed

    // remove all ASCII tab or newline from URL
    simple_buffer<CharT> buff_no_ws;
    do_remove_whitespace(first, last, buff_no_ws);
    //TODO-WARN: syntax violation if removed

    // reset
    clear();

    // reserve size (TODO: bet jei bus naudojama base?)
    auto length = std::distance(first, last);
    norm_url_.reserve(length + 32); // žr.: GURL::InitCanonical(..)

    const char* encoding = "UTF-8";
    // TODO: If encoding override is given, set encoding to the result of getting an output encoding from encoding override. 

    auto pointer = first;
    enum State {
        state_not_set = 0,
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
    State state = scheme_start_state;
    State state_override = state_not_set;

    // has scheme?
    if (state == scheme_start_state) {
        if (pointer != last && is_first_scheme_char(*pointer)) {
            state = scheme_state; // this appends first char to buffer
        } else if (!state_override) {
            state = no_scheme_state;
        } else {
            //TODO-ERR: syntax violation
            return false;
        }
    }

    if (state == scheme_state) {
        bool is_scheme = false;
        state = no_scheme_state;
        auto it_colon = std::find(pointer, last, ':');
        if (it_colon < last) {
            is_scheme = true;
            // start of scheme
            std::size_t norm_len0 = norm_url_.length();
            // first char
            norm_url_.push_back(detail::kSchemeCanonical[*pointer]);
            // other chars
            for (auto it = pointer + 1; it < it_colon; it++) {
                UCharT ch = static_cast<UCharT>(*it);
                char c = (ch < 0x80) ? detail::kSchemeCanonical[ch] : 0;
                if (c) {
                    norm_url_.push_back(c);
                } else {
                    is_scheme = false;
                    break;
                }
            }
            if (is_scheme) {
                pointer = it_colon + 1; // skip ':'
                set_scheme(norm_len0, norm_url_.length());
                norm_url_.push_back(':');
                // kas toliau
                //if (state_override) {
                //TODO:
                // 1. If state override is given, run these subsubsteps:
                //    1. If url’s scheme is a special scheme and buffer is not, terminate this algorithm.
                //    2. If url’s scheme is not a special scheme and buffer is, terminate this algorithm.
                // 2. Set url’s scheme to buffer.
                // 4. If state override is given, terminate this algorithm.
                //}
                if (is_file_scheme()) {
                    // TODO-WARN: if remaining does not start with "//", syntax violation.
                    state = file_state;
                } else {
                    if (is_special_scheme()) {
                        if (base && get_part_view(SCHEME).equal(base->get_part_view(SCHEME))) {
                            // NOTE: This means that base’s cannot-be-a-base-URL flag is unset
                            state = special_relative_or_authority_state;
                        } else {
                            state = special_authority_slashes_state;
                        }
                    } else if (pointer < last && *pointer == '/') {
                        state = path_or_authority_state;
                        pointer++;
                    } else {
                        cannot_be_base(true);
                        //TODO: append an empty string to url’s path
                        state = cannot_be_base_URL_path_state;
                    }
                }
            } else {
                // remove invalid scheme
                norm_url_.resize(norm_len0);
            }
        }
        if (state_override && !is_scheme) {
            //TODO-ERR: syntax violation
            return false;
        }
    }

    if (state == no_scheme_state) {
        if (base) {
            if (base->cannot_be_base()) {
                if (pointer < last && *pointer == '#') {
                    set_scheme(*base);
                    if_file_add_slash_slash();
                    append_parts(*base, PATH, QUERY);
                    //TODO: url’s fragment to the empty string
                    cannot_be_base(true);
                    state = fragment_state;
                    pointer++;
                } else {
                    // TODO-ERR: 1. syntax violation
                    return false;
                }
            } else {
                state = base->is_file_scheme() ? file_state : relative_state;
            }
        } else {
            //TODO-ERR: 1. syntax violation
            return false;
        }
    }
    
    if (state == special_relative_or_authority_state) {
        if (pointer + 1 < last && pointer[0] == '/' && pointer[1] == '/') {
            state = special_authority_ignore_slashes_state;
            pointer += 2; // skip "//"
        } else {
            //TODO-WARN: syntax violation
            state = relative_state;
        }
    }

    if (state == path_or_authority_state) {
        if (pointer < last && pointer[0] == '/') {
            state = authority_state;
            pointer++; // skip "/"
        } else {
            state = path_state;
        }
    }

    if (state == relative_state) {
        // std::assert(base != nullptr);
        set_scheme(*base);
        if_file_add_slash_slash();
        if (pointer == last) {
            // EOF code point
            // Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
            // url’s port to base’s port, url’s path to base’s path, and url’s query to base’s query
            append_parts(*base, USERNAME, QUERY);
            return true; // EOF
        } else {
            const CharT ch = *(pointer++);
            switch (ch) {
            case '/':
                state = relative_slash_state;
                break;
            case '?':
                // Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
                // url’s port to base’s port, url’s path to base’s path, url’s query to the empty string, and state to query state.
                append_parts(*base, USERNAME, PATH);
                state = query_state;    // sets query to the empty string
                break;
            case '#':
                // Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
                // url’s port to base’s port, url’s path to base’s path, url’s query to base’s query, url’s fragment to the empty string
                append_parts(*base, USERNAME, QUERY);
                state = fragment_state; // sets fragment to the empty string
                break;
            case '\\':
                if (is_special_scheme()) {
                    //TODO-WARN: syntax violation
                    state = relative_slash_state;
                    break;
                }
            default:
                // Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
                // url’s port to base’s port, url’s path to base’s path, and then remove url’s path’s last entry, if any
                append_parts(*base, USERNAME, PATH, base->get_path_rem_last());
                state = path_state;
                pointer--;
            }
        }
    }

    if (state == relative_slash_state) {
        const CharT ch = pointer < last ? pointer[0] : 0;
        if (ch == '/' || (ch == '\\' && is_special_scheme())) {
            // if (ch == '\\') // TODO-WARN: syntax violation;
            state = special_authority_ignore_slashes_state;
            pointer++;
        } else {
            // set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
            // url’s port to base’s port
            append_parts(*base, USERNAME, PORT);
            state = path_state;
        }
    }

    if (state == special_authority_slashes_state) {
        if (pointer + 1 < last && pointer[0] == '/' && pointer[1] == '/') {
            state = special_authority_ignore_slashes_state;
            pointer += 2; // skip "//"
        } else {
            //TODO-WARN: syntax violation
            state = special_authority_ignore_slashes_state;
        }
    }

    if (state == special_authority_ignore_slashes_state) {
        auto it = pointer;
        while (it < last && is_slash(*it)) it++;
        // if (it != pointer) // TODO-WARN: syntax violation
        pointer = it;
        state = authority_state;
    }

    // TODO?: credentials serialization do after host parsing,
    //       because if host null ==> then no credentials serialization
    if (state == authority_state) {
        // TODO: saugoti end_of_authority ir naudoti kituose state
        auto end_of_authority = is_special_scheme() ?
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#' || c == '\\'; }) :
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#'; });
        
        auto it_eta = find_last(pointer, end_of_authority, '@');
        if (it_eta != end_of_authority) {
            //TODO-WARN: syntax violation
            auto it_colon = std::find(pointer, it_eta, ':');
            // url includes credentials?
            const bool not_empty_password = std::distance(it_colon, it_eta) > 1;
            if (not_empty_password || std::distance(pointer, it_colon) > 0 /*not empty username*/) {
                // username
                add_slash_slash();
                std::size_t norm_len0 = norm_url_.length();
                detail::AppendStringOfType(pointer, it_colon, detail::CHAR_USERINFO, norm_url_); // UTF-8 percent encode, @ -> %40
                set_part(USERNAME, norm_len0, norm_url_.length());
                // password
                if (not_empty_password) {
                    norm_url_.push_back(':');
                    norm_len0 = norm_url_.length();
                    detail::AppendStringOfType(it_colon + 1, it_eta, detail::CHAR_USERINFO, norm_url_); // UTF-8 percent encode, @ -> %40
                    set_part(PASSWORD, norm_len0, norm_url_.length());
                }
                norm_url_ += '@';
            }
            // after '@'
            pointer = it_eta + 1;
        }
        state = host_state;
    }

    if (state == host_state || state == hostname_state) {
        auto end_of_authority = is_special_scheme() ?
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#' || c == '\\'; }) :
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#'; });

        bool in_square_brackets = false; // [] flag
        bool is_port = false;
        auto it_host_end = pointer;
        for (; it_host_end < end_of_authority; it_host_end++) {
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

        if (is_special_scheme() && pointer == it_host_end) {
            // TODE-ERR: host failure
            return false;
        }
        // parse and set host:
        add_slash_slash();
        if (!parse_host(pointer, it_host_end))
            return false;

        if (is_port) {
            pointer = it_host_end + 1; // skip ':'
            state = port_state;
            if (state_override == hostname_state)
                return true;
        } else {
            pointer = it_host_end;
            state = path_start_state;
            if (state_override)
                return true;
        }
    }

    if (state == port_state) {
        auto end_of_authority = is_special_scheme() ?
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#' || c == '\\'; }) :
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#'; });

        auto end_of_digits = std::find_if_not(pointer, end_of_authority, is_ascii_digit<CharT>);

        if (end_of_digits == end_of_authority || state_override) {
            if (pointer < end_of_digits) {
                // is port
                int port = 0;
                for (auto it = pointer; it < end_of_digits; it++) {
                    port = port * 10 + (*it - '0');
                    if (port > 0xFFFF) return false; // TODO-ERR: (2-1-2) syntax violation, failure
                }
                // set port if not default
                if (scheme_inf_ == nullptr || scheme_inf_->default_port != port) {
                    norm_url_.push_back(':');
                    add_part(PORT, std::to_string(port));
                    set_flag(PORT_FLAG);
                } //TODO: (2-1-3) Set url’s port to null
            }
            if (state_override)
                return true; // (2-2)
            state = path_start_state;
            pointer = end_of_authority;
        } else {
            // TODO-ERR: (3) syntax violation, failure
            return false;
        }
    }

    if (state == file_state) {
        if (!is_file_scheme())
            set_scheme({ "file", 4 });
        add_slash_slash(); // nes file:
        if (pointer == last) {
            // EOF code point
            if (base && base->is_file_scheme()) {
                // set url’s host to base’s host, url’s path to base’s path, and url’s query to base’s query
                append_parts(*base, HOST, QUERY);
                return true; // EOF
            }
            //TODO: čia neaišku ar "return true; // EOF" ar:
            state = path_state;
        } else {
            const CharT ch = *(pointer++);
            switch (ch) {
            case '\\':
                // TODO-WARN: syntax violation
            case '/':
                state = file_slash_state;
                break;
            case '?':
                if (base && base->is_file_scheme()) {
                    // set url’s host to base’s host, url’s path to base’s path, url’s query to the empty string
                    append_parts(*base, HOST, PATH);
                    state = query_state; // sets query to the empty string
                } else {
                    //TODO: čia neaišku ar ignoruoti ch ir cikle imti kitą ar:
                    state = path_state;
                    pointer--;
                }
                break;
            case '#':
                if (base && base->is_file_scheme()) {
                    // set url’s host to base’s host, url’s path to base’s path, url’s query to base’s query, url’s fragment to the empty string
                    append_parts(*base, HOST, QUERY);
                    state = fragment_state; // sets fragment to the empty string
                } else {
                    //TODO: čia neaišku ar ignoruoti ch ir cikle imti kitą ar:
                    state = path_state;
                    pointer--;
                }
                break;
            default:
                if (base && base->is_file_scheme()) {
                    if (pointer + 1 == last // remaining consists of one code point
                        || (pointer + 1 < last && !is_Windows_drive(ch, pointer[0]))
                        || (pointer + 2 < last && !is_special_authority_end_char(pointer[1]))
                        ) {
                        // set url’s host to base’s host, url’s path to base’s path, and then shorten url’s path
                        append_parts(*base, HOST, PATH, base->get_shorten_path());
                        // Note: This is a (platform-independent) Windows drive letter quirk.
                    }
                    // else // TODO-WARN: syntax violation
                }
                state = path_state;
                pointer--;
            }
        }
    }

    if (state == file_slash_state) {
        // EOF ==> 0 ==> default:
        switch (pointer != last ? *pointer : 0) {
        case '\\':
            // TODO-WARN: syntax violation
        case '/':
            state = file_host_state;
            pointer++;
            break;

        default:
            if (base && base->is_file_scheme()) {
                str_view<char> base_path = base->get_part_view(url::PATH);
                // TODO?: patikrinti ar gerai (ar yra base_path[0] == '/'):
                // if base’s path first string is a normalized Windows drive letter
                if ((base_path.length() == 3 || (base_path.length() > 3 && base_path[3] == '/'))
                    && is_normalized_Windows_drive(base_path[1], base_path[2])
                    ) {
                    // append base’s path first string to url’s path
                    append_parts(*base, PATH, PATH, detail::url_part(base->part_[PATH].offset, 3)); //TODO: gal galima apseiti be šios f-jos?
                    // Note: This is a (platform - independent) Windows drive letter quirk.
                    // Both url’s and base’s host are null under these conditions and therefore not copied.
                }
            }
            state = path_state;
        }
    }

    if (state == file_host_state) {
        auto end_of_authority = std::find_if(pointer, last, is_special_authority_end_char<CharT>);

        if (pointer == end_of_authority) {
            state = path_start_state;
        } else if (pointer + 2 == end_of_authority && is_Windows_drive(pointer[0], pointer[1])) {
            // buffer is a Windows drive letter
            // TODO-WARN: syntax violation
            state = path_state;
            // Note: This is a (platform - independent) Windows drive letter quirk.
            // buffer is not reset here and instead used in the path state.
            // TODO: buffer is not reset here and instead used in the path state
        } else {
            // parse and set host:
            add_slash_slash(); // TODO: gal nebereikia?
            if (!parse_host(pointer, end_of_authority))
                return false; // failure
            // If host is not "localhost", set url’s host to host
            if (get_part_view(HOST).equal({ "localhost", 9 })) {
                clear_host();
            }
            pointer = end_of_authority;
            state = path_start_state;
        }
    }

    if (state == path_start_state) {
        if (pointer != last) {
            const CharT ch = *pointer;
            if (ch == '/') pointer++;
            else if (is_special_scheme() && ch == '\\') {
                // TODO-WARN: syntax violation
                pointer++;
            }
        }
        state = path_state;
    }

    if (state == path_state) {
        // TODO: saugoti end_of_path ir naudoti kituose state
        auto end_of_path = state_override ? last :
            std::find_if(pointer, last, [](CharT c) { return c == '?' || c == '#'; });

        parse_path(pointer, end_of_path);
        pointer = end_of_path;
        
        if (pointer == last) {
            return true;
        } else {
            const CharT ch = *pointer++;
            if (ch == '?') {
                // TODO: set url’s query to the empty string
                state = query_state;
            } else {
                // ch == '#'
                // TODO: set url’s fragment to the empty string
                state = fragment_state;
            }
        }
    }

    if (state == cannot_be_base_URL_path_state) {
        auto end_of_path = state_override ? last :
            std::find_if(pointer, last, [](CharT c) { return c == '?' || c == '#'; });

        // UTF-8 percent encode using the simple encode set, and append the result
        // to the first string in url’s path
        part_[PATH].offset = norm_url_.length();
        do_simple_path(pointer, end_of_path, norm_url_);
        part_[PATH].len = norm_url_.length() - part_[PATH].offset;
        pointer = end_of_path;

        if (pointer == last) {
            return true;
        } else {
            const CharT ch = *pointer++;
            if (ch == '?') {
                // TODO: set url’s query to the empty string
                state = query_state;
            } else {
                // ch == '#'
                // TODO: set url’s fragment to the empty string
                state = fragment_state;
            }
        }
    }

    if (state == query_state) {
        auto end_of_query = state_override ? last : std::find(pointer, last, '#');

        // TODO-WARN:
        //for (auto it = pointer; it < end_of_query; it++) {
        //  UCharT c = static_cast<UCharT>(*it);
        //  // 1. If c is not a URL code point and not "%", syntax violation.
        //  // 2. If c is "%" and remaining does not start with two ASCII hex digits, syntax violation.
        //}

        // scheme_inf_ == nullptr, if unknown scheme
        if (!scheme_inf_ || !scheme_inf_->is_special || scheme_inf_->is_ws)
            encoding = "UTF-8";

        // Set buffer to the result of encoding buffer using encoding
        // percent encode: (c < 0x21 || c > 0x7E || c == 0x22 || c == 0x23 || c == 0x3C || c == 0x3E)
        // TODO: dabar palaiko tik encoding = "UTF-8"; kitų palaikymą galima padaryti pagal:
        // https://cs.chromium.org/chromium/src/url/url_canon_query.cc?rcl=1479817139&l=93
        norm_url_.push_back('?');
        std::size_t norm_len0 = norm_url_.length();
        detail::AppendStringOfType(pointer, end_of_query, detail::CHAR_QUERY, norm_url_); // šis dar koduoja 0x27 '
        set_part(QUERY, norm_len0, norm_url_.length());
        set_flag(QUERY_FLAG);

        pointer = end_of_query;
        if (pointer == last)
            return true; // EOF
        // *pointer == '#'
        //TODO: set url’s fragment to the empty string
        state = fragment_state;
        pointer++; // skip '#'
    }

    if (state == fragment_state) {
        norm_url_ += '#';
        // pagal: https://cs.chromium.org/chromium/src/url/url_canon_etc.cc : DoCanonicalizeRef(..)
        std::size_t norm_len0 = norm_url_.length();
        while (pointer < last) {
            // UTF-8 percent encode c using the simple encode set & ignore '\0'
            UCharT uch = static_cast<UCharT>(*pointer);
            if (uch >= 0x80) {
                // invalid utf-8/16/32 sequences will be replaced with kUnicodeReplacementCharacter
                detail::AppendUTF8EscapedChar(pointer, last, norm_url_);
            } else {
                if (uch >= 0x20) {
                    // Normal ASCII characters are just appended
                    norm_url_.push_back(static_cast<unsigned char>(uch));
                } else if (uch) {
                    // C0 control characters (except '\0') are escaped
                    detail::AppendEscapedChar(uch, norm_url_);
                } else { // uch == 0
                    // TODO-WARN: Syntax violation
                }
                pointer++;
            }
            // TODO-WARN:
            // If c is not a URL code point and not "%", syntax violation.
            // If c is "%" and remaining does not start with two ASCII hex digits, syntax violation.
        }
        set_part(FRAGMENT, norm_len0, norm_url_.length());
        set_flag(FRAGMENT_FLAG);
    }

    return true;
}

// internal functions

template <typename CharT>
static inline bool is_valid_host_chars(const CharT* first, const CharT* last) {
    return std::none_of(first, last, detail::IsInvalidHostChar);
}

template <typename CharT>
inline bool url::parse_host(const CharT* first, const CharT* last) {
    typedef std::make_unsigned<CharT>::type UCharT;

    // ši sąlyga turi būti patikrinta prieš kreipiantis
    // (todo angl?: this condition must by verified before calling)
    if (first == last) {
        // https://github.com/whatwg/url/issues/79
        // https://github.com/whatwg/url/pull/189
        // set empty host
        const std::size_t norm_len0 = norm_url_.length();
        set_part(HOST, norm_len0, norm_len0);
        set_flag(HOST_FLAG);
        return true;
    }
    assert(first < last);

    if (*first == '[') {
        if (*(last - 1) == ']') {
            return parse_ipv6(first + 1, last - 1);
        } else {
            // TODO-ERR: syntax violation
            return false;
        }
    }

    // check if host has non ascii characters or percent sign
    bool has_no_ascii = false;
    bool has_escaped = false;
    for (auto it = first; it < last;) {
        UCharT uch = static_cast<UCharT>(*it++);
        if (uch >= 0x80) {
            has_no_ascii = true;
        } else {
            unsigned char uc8;
            if (uch == '%' && detail::DecodeEscaped(it, last, uc8)) {
                has_escaped = true;
                if ((has_no_ascii = has_no_ascii || (uc8 >= 0x80))) break;
            }
        }
    }

    //TODO: klaidų nustatymas pagal standartą

    std::basic_string<char16_t> buff_uc;
    if (has_no_ascii) {
        if (has_escaped) {
            simple_buffer<unsigned char> buff_utf8;

            uint32_t code_point;
            for (auto it = first; it < last;) {
                url_util::read_utf_char(it, last, code_point);
                if (code_point == '%') {
                    // unescape until escaped
                    unsigned char uc8;
                    if (detail::DecodeEscaped(it, last, uc8)) {
                        buff_utf8.push_back(uc8);
                        while (it < last && *it == '%') {
                            it++; // skip '%'
                            if (!detail::DecodeEscaped(it, last, uc8))
                                uc8 = '%';
                            buff_utf8.push_back(uc8);
                        }
                        detail::ConvertUTF8ToUTF16(buff_utf8.data(), buff_utf8.data() + buff_utf8.size(), buff_uc);
                        buff_utf8.clear();
                    } else {
                        detail::AppendUTF16Value(code_point, buff_uc);
                    }
                } else {
                    detail::AppendUTF16Value(code_point, buff_uc);
                }
            }
        } else {
            detail::ConvertToUTF16(first, last, buff_uc);
        }
    } else {
        // (first,last) has only ASCII characters
        // Net ir ASCII turi praeiti IDNToASCII patikrinimą;
        // tačiau žr.: https://github.com/jsdom/whatwg-url/issues/50
        //  ^-- kad korektiškai veiktų reikia Unicode 9 palaikymo
        if (has_escaped) {
            for (auto it = first; it < last;) {
                unsigned char uc8 = static_cast<unsigned char>(*it++);
                if (uc8 == '%')
                    detail::DecodeEscaped(it, last, uc8);
                buff_uc.push_back(uc8);
            }
        } else {
            buff_uc.append(first, last);
        }
    }

    // domain to ASCII
    simple_buffer<char16_t> buff_ascii;

    if (!IDNToASCII(buff_uc.data(), buff_uc.length(), buff_ascii))
        return false;
    if (!is_valid_host_chars(buff_ascii.data(), buff_ascii.data() + buff_ascii.size())) {
        //TODO-ERR: syntax violation
        return false;
    }
    // IPv4
    const ParseResult res = parse_ipv4(buff_ascii.begin(), buff_ascii.end());
    if (res == RES_FALSE) {
        std::size_t norm_len0 = norm_url_.length();
        norm_url_.append(buff_ascii.begin(), buff_ascii.end());
        set_part(HOST, norm_len0, norm_url_.length());
        set_flag(HOST_FLAG);
    }
    return res != RES_ERROR;
}

template <typename CharT>
inline ParseResult url::parse_ipv4(const CharT* first, const CharT* last) {
    uint32_t ipv4;

    const ParseResult res = ipv4_parse(first, last, ipv4);
    if (res == RES_OK) {
        std::size_t norm_len0 = norm_url_.length();
        ipv4_serialize(ipv4, norm_url_);
        set_part(HOST, norm_len0, norm_url_.length());
        set_flag(HOST_FLAG);
    }
    return res;
}

template <typename CharT>
inline bool url::parse_ipv6(const CharT* first, const CharT* last) {
    uint16_t ipv6addr[8];

    if (ipv6_parse(first, last, ipv6addr)) {
        std::size_t norm_len0 = norm_url_.length();
        norm_url_.push_back('[');
        ipv6_serialize(ipv6addr, norm_url_);
        norm_url_.push_back(']');
        set_part(HOST, norm_len0, norm_url_.length());
        set_flag(HOST_FLAG);
        return true;
    }
    return false;
}

template <typename CharT>
inline void url::parse_path(const CharT* first, const CharT* last) {
    // path state; includes:
    // 1. [ (/,\) - 1, 2, 3, 4 - [ 1 (if first segment), 2 ] ]
    // 2. [ 1 ... 4 ]
    auto double_dot = [](const CharT* const pointer, const int len) -> bool {
        if (len == 2) {
            return (pointer[0] == '.' && pointer[1] == '.');
        } else if (len == 4) {
            return detail::AsciiEqualNoCase(pointer, pointer + 4, ".%2e") ||
                   detail::AsciiEqualNoCase(pointer, pointer + 4, "%2e.");
        } else if (len == 6) {
            return detail::AsciiEqualNoCase(pointer, pointer + 6, "%2e%2e");
        } else
            return false;
    };
    auto single_dot = [](const CharT* const pointer, const int len) -> bool {
        if (len == 1) {
            return pointer[0] == '.';
        } else if (len == 3) {
            return detail::AsciiEqualNoCase(pointer, pointer + 3, "%2e");
        } else
            return false;
    };

    // start of path
    if (is_empty(PATH))
        part_[PATH].offset = norm_url_.length();

    auto pointer = first;
    while (true) {
        auto end_of_segment = is_special_scheme()
            ? std::find_if(pointer, last, is_slash<CharT>)
            : std::find(pointer, last, '/');

        int len = end_of_segment - pointer;
        bool is_last = end_of_segment == last;
        // TODO-WARN: 1. If url is special and c is "\", syntax violation.

        if (double_dot(pointer, len)) {
            shorten_path();
            if (is_last) append_to_path();
        } else if (single_dot(pointer, len)) {
            if (is_last) append_to_path();
        } else {
            if (len == 2 &&
                is_file_scheme() &&
                part_[PATH].empty() &&
                is_Windows_drive(pointer[0], pointer[1]))
            {
                if (!is_null(HOST)) {
                    // 1. If url’s host is non-null, syntax violation
                    // TODO-WARN: syntax violation
                    // 2. Set url’s host to null
                    clear_host();
                    // fix offset
                    part_[PATH].offset = norm_url_.length();
                }
                // and replace the second code point in buffer with ":"
                norm_url_ += '/';
                norm_url_ += static_cast<char>(pointer[0]);
                norm_url_ += ':';
                //Note: This is a (platform-independent) Windows drive letter quirk.
            } else {
                norm_url_ += '/';
                do_path_segment(pointer, end_of_segment, norm_url_);
                pointer = end_of_segment;
            }
            // sutvarkom kelio ilgį
            part_[PATH].len = norm_url_.length() - part_[PATH].offset;
        }
        // next segment
        if (is_last) break;
        pointer = end_of_segment + 1; // skip '/' or '\'
    }
}

template <typename CharT>
inline bool url::do_path_segment(const CharT* pointer, const CharT* last, std::string& output) {
    typedef std::make_unsigned<CharT>::type UCharT;

    // TODO-WARN: 2. [ 1 ... 2 ] syntax violation.
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
            if (!IsCharOfType(uc, detail::CHAR_DEFAULT))
                detail::AppendEscapedChar(uc, output);
            else
                output.push_back(uc);
            pointer++;
        }
    }
    return success;
}

template <typename CharT>
inline bool url::do_simple_path(const CharT* pointer, const CharT* last, std::string& output) {
    typedef std::make_unsigned<CharT>::type UCharT;

    // 3. of "cannot-be-a-base-URL path state"
    // TODO-WARN: 3. [ 1 ... 2 ] syntax violation.
    //  1. If c is not EOF code point, not a URL code point, and not "%", syntax violation.
    //  2. If c is "%" and remaining does not start with two ASCII hex digits, syntax violation.

    bool success = true;
    while (pointer < last) {
        // UTF-8 percent encode c using the simple encode set
        UCharT uch = static_cast<UCharT>(*pointer);
        if (uch >= 0x80) {
            // invalid utf-8/16/32 sequences will be replaced with 0xfffd
            success &= detail::AppendUTF8EscapedChar(pointer, last, output);
        } else {
            // Just append the 7-bit character, escaping C0 control chars:
            if (uch <= 0x1f)
                detail::AppendEscapedChar(uch, output);
            else
                output.push_back(static_cast<unsigned char>(uch));
            pointer++;
        }
    }
    return success;
}

inline detail::url_part url::get_path_rem_last() const {
    if (!part_[PATH].empty()) {
        const char* first = norm_url_.data() + part_[PATH].offset;
        const char* last = first + part_[PATH].len;
        const char* it = find_last(first, last, '/');
        if (it == last) it = first; // jei nebuvo '/' išmesim visą kelią
        // return shorten
        return detail::url_part(part_[PATH].offset, it - first);
    }
    return part_[PATH];
}

inline detail::url_part url::get_shorten_path() const {
    if (!part_[PATH].empty()) {
        if (!is_file_scheme() ||
            part_[PATH].len != 3 ||
            !is_normalized_Windows_drive(norm_url_[part_[PATH].offset + 1], norm_url_[part_[PATH].offset + 2]))
        {
            const char* first = norm_url_.data() + part_[PATH].offset;
            const char* last = first + part_[PATH].len;
            const char* it = find_last(first, last, '/');
            if (it == last) it = first; // jei nebuvo '/' išmesim visą kelią
            // return shorten
            return detail::url_part(part_[PATH].offset, it - first);
        }
    }
    return part_[PATH];
}

inline void url::shorten_path() {
    if (!part_[PATH].empty()) {
        if (!is_file_scheme() ||
            part_[PATH].len != 3 ||
            !is_normalized_Windows_drive(norm_url_[part_[PATH].offset + 1], norm_url_[part_[PATH].offset + 2]))
        {
            const char* first = norm_url_.data() + part_[PATH].offset;
            const char* last = first + part_[PATH].len;
            const char* it = find_last(first, last, '/');
            if (it == last) it = first; // jei nebuvo '/' išmesim visą kelią
            // shorten
            part_[PATH].len = it - first;
            norm_url_.resize(it - norm_url_.data());
        }
    }
}


inline bool url::get_path_rem_last(detail::url_part& part, unsigned& path_segment_count) const {
    if (path_segment_count_ > 0) {
        // Remove path’s last item
        const char* first = norm_url_.data() + part_[url::PATH].offset;
        const char* last = first + part_[url::PATH].len;
        const char* it = find_last(first, last, '/');
        if (it == last) it = first; // jei nebuvo '/' išmesim visą kelią
        // shorten
        part.offset = part_[url::PATH].offset;
        part.len = it - first;
        path_segment_count = path_segment_count_ - 1;
        return true;
    }
    return false;
}

inline bool url::get_shorten_path(detail::url_part& part, unsigned& path_segment_count) const {
    if (path_segment_count_ == 0 || (
        is_file_scheme() &&
        path_segment_count_ == 1 &&
        part_[url::PATH].len == 2 &&
        is_normalized_Windows_drive(norm_url_[part_[url::PATH].offset], norm_url_[part_[url::PATH].offset + 1])
        ))
        return false;
    // Remove path’s last item
    return get_path_rem_last(part, path_segment_count);
}

inline void url_serializer::shorten_path() {
    assert(last_pt_ <= url::PATH);
    if (url_.get_shorten_path(url_.part_[url::PATH], url_.path_segment_count_))
        url_.norm_url_.resize(url_.part_[url::PATH].offset + url_.part_[url::PATH].len);
}

inline void url::append_to_path() {
    if (part_[PATH].len <= 0) {
        part_[PATH].offset = norm_url_.length();
        part_[PATH].len = 0;
    }
    norm_url_.push_back('/');
    part_[PATH].len++;
}

// append parts from other url

inline void url::append_parts(const url& src, PartType t1, PartType t2, detail::url_part lastp) {
    // See URL serializing
    // https://url.spec.whatwg.org/#concept-url-serializer
    PartType ifirst;
    if (t1 <= HOST) {
        // authority, host
        if (!src.is_null(HOST)) {
            // If url’s host is non - null, append "//" to output
            add_slash_slash();
            if (t1 == USERNAME && src.has_credentials())
                ifirst = USERNAME;
            else
                ifirst = HOST;
        } else {
            // if url’s host is null and url’s scheme is "file", append "//" to output
            // TODO: gal nebereikia, nes iškviečiamas prieš append_parts(..) bei netiesiogiai
            //       kai relative_slash_state, file_slash_state
            if_file_add_slash_slash();

            ifirst = PATH;
        }
    } else {
        // t1 == PATH
        ifirst = t1;
    }

    // copy parts & str
    if (ifirst <= t2) {
        int ilast = t2;
        if (lastp.offset == 0) {
            for (; ilast >= t1; ilast--) {
                if (src.part_[ilast].offset) {
                    lastp = src.part_[ilast];
                    break;
                }
            }
        }
        if (lastp.offset) {
            const char* first = src.norm_url_.data() + src.part_[ifirst].offset;
            const char* last = src.norm_url_.data() + lastp.offset + lastp.len;
            int delta = static_cast<int>(norm_url_.length()) - src.part_[ifirst].offset;
            norm_url_.append(first, last);
            for (int ind = ifirst; ind < ilast; ind++) {
                if (src.part_[ind].offset)
                    part_[ind] = detail::url_part(src.part_[ind].offset + delta, src.part_[ind].len);
            }
            // ilast - lastp
            part_[ilast] = detail::url_part(lastp.offset + delta, lastp.len);
        }
    }

    // copy not null flags
    unsigned mask = 0;
    for (int ind = t1; ind <= t2; ind++) {
        mask |= (1u << ind);
    }
    flags_ = (flags_ & ~mask) | (src.flags_ & mask);
}



// class url_serializer

inline url_serializer::~url_serializer() {
    switch (last_pt_) {
    case url::SCHEME:
        // if url’s host is null and url’s scheme is "file"
        if (url_.is_file_scheme())
            url_.norm_url_.append("//");
    case url::HOST:
    case url::PORT:
        // append '/' if not cannot-be-a-base-URL flag
        if (!url_.cannot_be_base())
            url_.norm_url_ += '/';
        break;
    }
}

inline std::string& url_serializer::start_part(url::PartType t) {
    switch (last_pt_) {
    case url::SCHEME:
        // if host is non-null or scheme is "file"
        if (t <= url::HOST || url_.is_file_scheme())
            url_.norm_url_.append("//");
        // append '/' if not cannot-be-a-base-URL flag
        if (t >= url::PATH && !url_.cannot_be_base())
            url_.norm_url_ += '/';
        break;
    case url::USERNAME:
        if (t == url::PASSWORD) {
            url_.norm_url_ += ':';
            break;
        }
    case url::PASSWORD:
        if (t == url::HOST) {
            url_.norm_url_ += '@';
            break;
        }
        // NOT REACHABLE (TODO: throw?)
        break;
    case url::HOST:
        if (t == url::PORT) {
            url_.norm_url_ += ':';
            break;
        }
    case url::PORT:
        // append '/' if not cannot-be-a-base-URL flag
        if (t >= url::PATH && !url_.cannot_be_base())
            url_.norm_url_ += '/';
        break;
    case url::PATH:
        if (t == url::PATH) // continue on path
            return url_.norm_url_;
        break;
    }

    switch (t) {
    case url::QUERY:
        url_.norm_url_ += '?';
        break;
    case url::FRAGMENT:
        url_.norm_url_ += '#';
        break;
    }

    assert(last_pt_ < t);
    url_.part_[last_pt_ = t].offset = url_.norm_url_.length();
    return url_.norm_url_;
}

inline void url_serializer::save_part() {
    url_.part_[last_pt_].len = url_.norm_url_.length() - url_.part_[last_pt_].offset;
}

// append_empty_to_path() kviečiamas trijose vietose:
// 1) scheme_state -> [2.9.] -> cannot_be_base_URL_path_state
// 2) path_state -> [1.2. ".." ]
// 3) path_state -> [1.3. "." ]
inline void url_serializer::append_to_path() { // append_empty_to_path();
    start_path_segment();
    save_path_segment();
}

inline std::string& url_serializer::start_path_segment() {
    if (last_pt_ != url::PATH)
        return start_part(url::PATH);
    // lipdom prie esamo kelio
    url_.norm_url_ += '/';
    return url_.norm_url_;
}

inline void url_serializer::save_path_segment() {
    save_part();
    url_.path_segment_count_++;
}

inline void url_serializer::clear_host() {
    assert(!is_empty(url::SCHEME));
    assert(last_pt_ == url::HOST);
    // paliekam tik "scheme:"   
    url_.norm_url_.resize(url_.part_[url::SCHEME].len + 1);
    url_.part_[url::HOST].offset = 0;
    url_.part_[url::HOST].len = 0;
    url_.flags_ &= ~url::HOST_FLAG; // set to null
    last_pt_ = url::SCHEME;
}

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
            ifirst = url::PATH;
        }
    } else {
        // t1 == PATH
        ifirst = t1;
    }

    // copy parts & str
    if (ifirst <= t2) {
        int ilast = t2;
        for (; ilast >= ifirst; ilast--) {
            if (src.part_[ilast].offset)
                break;
        }
        if (ifirst <= ilast) {
            detail::url_part lastp(src.part_[ilast]);
            if (pathOpFn && ilast == url::PATH) {
                unsigned segment_count = src.path_segment_count_;
                // https://isocpp.org/wiki/faq/pointers-to-members
                // todo: use std::invoke (c++17)
                (src.*pathOpFn)(lastp, segment_count);
                url_.path_segment_count_ = segment_count;
            } else if (ifirst <= url::PATH && url::PATH <= ilast) {
                url_.path_segment_count_ = src.path_segment_count_;
            }
            // src
            const char* first = src.norm_url_.data() + src.part_[ifirst].offset;
            const char* last = src.norm_url_.data() + lastp.offset + lastp.len;
            // dest                     
            std::string& norm_url = start_part(ifirst);
            int delta = static_cast<int>(norm_url.length()) - src.part_[ifirst].offset;
            // copy normalized url string from src
            norm_url.append(first, last);
            // adjust url_.part_
            for (int ind = ifirst; ind < ilast; ind++) {
                if (src.part_[ind].offset)
                    url_.part_[ind] = detail::url_part(src.part_[ind].offset + delta, src.part_[ind].len);
            }
            // ilast part from lastp
            url_.part_[ilast] = detail::url_part(lastp.offset + delta, lastp.len);
        }
    }

    // copy not null flags
    unsigned mask = 0;
    for (int ind = t1; ind <= t2; ind++) {
        mask |= (1u << ind);
    }
    url_.flags_ = (url_.flags_ & ~mask) | (src.flags_ & mask);
}


} // namespace whatwg

#endif // WHATWG_URL_H
