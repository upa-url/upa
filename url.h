// Copyright 2016 Rimas Misevičius
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// URL Standard
// https://url.spec.whatwg.org/ [21 November 2016]
// https://url.spec.whatwg.org/commit-snapshots/56b74ce7cca8883eab62e9a12666e2fac665d03d/
// Infra Standard - fundamental concepts upon which standards are built
// https://infra.spec.whatwg.org/

#ifndef WHATWG_URL_H
#define WHATWG_URL_H

#include "url_canon.h"
#include "url_util.h"
#include <algorithm>
#include <cassert>
#include <string>
#include <type_traits>
#include <vector>

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
    int default_port;
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
        , cannot_be_base_(0)
    {}

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

    std::string get_part(PartType t) const {
        return std::string(norm_url_.data() + part_[t].offset, part_[t].len);
    }

    str_view<char> get_part_view(PartType t) const {
        return str_view<char>(norm_url_.data() + part_[t].offset, part_[t].len);
    }

    bool is_special_scheme() const {
        return scheme_inf_ && scheme_inf_->is_special;
    }

    bool is_file_scheme() const {
        return scheme_inf_ && scheme_inf_->is_file;
    }

protected:
    template <typename CharT>
    bool parse_host(const CharT* first, const CharT* last);

    template <typename CharT>
    void parse_path(const CharT* first, const CharT* last);

    template <typename CharT>
    bool do_path_segment(const CharT* pointer, const CharT* last, std::string& output);

    template <typename CharT>
    bool do_simple_path(const CharT* pointer, const CharT* last, std::string& output);

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

    void set_scheme(const url& src) {
        add_part(SCHEME, src.get_part_view(SCHEME), ':');
        scheme_inf_ = src.scheme_inf_;
    }
    void set_scheme(const str_view<char> str) {
        add_part(SCHEME, str, ':');
        scheme_inf_ = detail::get_scheme_info(str);
    }
    void set_scheme(std::size_t b, std::size_t e) {
        set_part(SCHEME, b, e);
        scheme_inf_ = detail::get_scheme_info(get_part_view(SCHEME));
    }


private:
    std::string norm_url_;
    detail::url_part part_[PART_COUNT];
    const detail::scheme_info* scheme_inf_;
    unsigned cannot_be_base_ : 1;
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
inline void do_remove_whitespace(const CharT*& first, const CharT*& last, std::vector<CharT>& buff) {
    // Fast verification that there’s nothing that needs removal. This is the 99%
    // case, so we want it to be fast and don't care about impacting the speed
    // when we do find whitespace.
    for (auto it = first; it < last; it++) {
        if (!is_removable_char(*it))
            continue;
        // copy non whitespace chars into the new buffer and return it
        buff.reserve(last - first);
        buff.insert(buff.begin(), first, it);
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


// https://url.spec.whatwg.org/#concept-basic-url-parser
template <typename CharT>
inline bool url::parse(const CharT* first, const CharT* last, const url* base) {
    typedef std::make_unsigned<CharT>::type UCharT;

    // remove any leading and trailing C0 control or space:
    do_trim(first, last);
    //TODO-WARN: syntax violation if trimmed

    // remove all ASCII tab or newline from URL
    std::vector<CharT> buff_no_ws;
    do_remove_whitespace(first, last, buff_no_ws);
    //TODO-WARN: syntax violation if removed

    // input is empty ar whitespace:
    // - must return empty URL
    // - TODO: return true or false?
    if (first == last)
        return true;

    // reserve size
    auto length = std::distance(first, last);
    norm_url_.reserve(length + 32); // žr.: GURL::InitCanonical(..)

    // reset
    norm_url_.resize(0);
    std::memset(part_, 0, sizeof(part_));
    scheme_inf_ = nullptr;
    cannot_be_base_ = 0;

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
    bool is_slash_slash = false;

    // has scheme?
    if (state == scheme_start_state) {
        bool is_scheme = false;
        state = no_scheme_state;
        if (is_first_scheme_char(*pointer)) {
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
                            cannot_be_base_ = 1;
                            //TODO: append an empty string to url’s path
                            state = cannot_be_base_URL_path_state;
                        }
                    }
                } else {
                    // remove invalid scheme
                    norm_url_.resize(norm_len0);
                }
            }
        }
        if (state_override && !is_scheme) {
            //TODO-ERR: syntax violation
            return false;
        }
    }

    if (state == no_scheme_state) {
        if (base) {
            if (base->cannot_be_base_) {
                if (pointer < last && *pointer == '#') {
                    set_scheme(*base);
                    add_part(PATH, base->get_part_view(PATH));
                    add_part(QUERY, base->get_part_view(QUERY));
                    //TODO: url’s fragment to the empty string
                    cannot_be_base_ = 1;
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
        if (pointer == last) {
            // EOF code point
            // TODO: Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
            // url’s port to base’s port, url’s path to base’s path, and url’s query to base’s query

            return true; // EOF
        } else {
            const CharT ch = *(pointer++);
            switch (ch) {
            case '/':
                state = relative_slash_state;
                break;
            case '?':
                // TODO: Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
                // url’s port to base’s port, url’s path to base’s path, url’s query to the empty string, and state to query state.
                state = query_state;
                break;
            case '#':
                // TODO: Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
                // url’s port to base’s port, url’s path to base’s path, url’s query to base’s query, url’s fragment to the empty string
                state = fragment_state;
                break;
            case '\\':
                if (is_special_scheme()) {
                    //TODO-WARN: syntax violation
                    state = relative_slash_state;
                    break;
                }
            default:
                // TODO: Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
                // url’s port to base’s port, url’s path to base’s path, and then remove url’s path’s last entry, if any
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
            // TODO: set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
            // url’s port to base’s port
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

    if (state == authority_state) {
        // TODO: saugoti end_of_authority ir naudoti kituose state
        auto end_of_authority = is_special_scheme() ?
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#' || c == '\\'; }) :
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#'; });
        
        auto it_eta = find_last(pointer, end_of_authority, '@');
        if (it_eta != end_of_authority) {
            //TODO-WARN: syntax violation
            auto it_colon = std::find(pointer, it_eta, ':');
            // username
            norm_url_.append("//");
            is_slash_slash = true; // kad nebūtų dar kartą pridėta "//"
            std::size_t norm_len0 = norm_url_.length();
            detail::AppendStringOfType(pointer, it_colon, detail::CHAR_USERINFO, norm_url_); // UTF-8 percent encode, @ -> %40
            set_part(USERNAME, norm_len0, norm_url_.length());
            // password
            if (it_colon != it_eta) {
                norm_url_.push_back(':');
                norm_len0 = norm_url_.length();
                detail::AppendStringOfType(it_colon + 1, it_eta, detail::CHAR_USERINFO, norm_url_); // UTF-8 percent encode, @ -> %40
                set_part(PASSWORD, norm_len0, norm_url_.length());
            }
            // after '@'
            pointer = it_eta + 1;
            norm_url_ += '@';
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
        if (!is_slash_slash) {
            is_slash_slash = true;
            norm_url_.append("//");
        }
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

        int port = 0;
        for (auto it = pointer; it < end_of_authority; it++) {
            const CharT ch = *it;
            if (is_ascii_digit(ch)) {
                port = port * 10 + (ch - '0');
                if (port > 0xFFFF) return false; // TODO-ERR: syntax violation, failure
            } else if (state_override) {
                break;
            } else {
                // TODO-ERR: syntax violation, failure
                return false;
            }
        }
        if (port) {
            if (scheme_inf_ == nullptr || scheme_inf_->default_port != port) {
                norm_url_.push_back(':');
                add_part(PORT, std::to_string(port));
            }
        }
        if (state_override)
            return true;
        state = path_start_state;
        pointer = end_of_authority;
    }

    if (state == file_state) {
        set_scheme({ "file", 4 });
        if (pointer == last) {
            // EOF code point
            if (base && base->is_file_scheme()) {
                // TODO: set url’s host to base’s host, url’s path to base’s path, and url’s query to base’s query
            }
            return true; // EOF
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
                    // TODO: set url’s host to base’s host, url’s path to base’s path, url’s query to the empty string
                    state = query_state;
                } else {
                    //TODO: čia neaišku ar ignoruoti ch ir cikle imti kitą ar:
                    state = path_state;
                    pointer--;
                }
                break;
            case '#':
                if (base && base->is_file_scheme()) {
                    // TODO: set url’s host to base’s host, url’s path to base’s path, url’s query to base’s query, url’s fragment to the empty string
                    state = fragment_state;
                } else {
                    //TODO: čia neaišku ar ignoruoti ch ir cikle imti kitą ar:
                    state = path_state;
                    pointer--;
                }
                break;
            default:
                if (base && base->is_file_scheme()) {
                    if (pointer + 1 == last // remaining consists of one code point
                        || (pointer + 1 > last && !is_Windows_drive(ch, pointer[0]))
                        || (pointer + 2 > last && !is_special_authority_end_char(pointer[1]))
                        ) {
                        // Note: This is a (platform-independent) Windows drive letter quirk.
                        // TODO: set url’s host to base’s host, url’s path to base’s path, and then shorten url’s path
                    }
                    // else // TODO-WARN: syntax violation
                }
                state = path_state;
                pointer--;
            }
        }
    }

    if (state == file_slash_state) {
        if (pointer == last) return true; // EOF

        switch (*pointer) {
        case '\\':
            // TODO-WARN: syntax violation
        case '/':
            state = file_host_state;
            pointer++;
            break;

        default:
            if (base && base->is_file_scheme()) {
                str_view<char> base_path = base->get_part_view(url::PATH);
                //TODO: patikrinti ar gerai (ar yra base_path[0] == '/') :
                if (base_path.length() >= 3 && is_normalized_Windows_drive(base_path[1], base_path[2])) {
                    // Note: This is a (platform - independent) Windows drive letter quirk. Both url’s and base’s host
                    // are null under these conditions and therefore not copied.
                    // TODO: append base’s path first string to url’s path
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
            // TODO: If host is not "localhost", set url’s host to host
            if (!parse_host(pointer, end_of_authority))
                return false; // failure
            state = path_start_state;
        }
    }

    if (state == path_start_state) {
        if (pointer == last) return true; // EOF

        const CharT ch = *pointer;
        if (ch == '/') pointer++;
        else if (is_special_scheme() && ch == '\\') {
            // TODO-WARN: syntax violation
            pointer++;
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

        if (!scheme_inf_->is_special || scheme_inf_->is_ws)
            encoding = "UTF-8";

        // Set buffer to the result of encoding buffer using encoding
        // percent encode: (c < 0x21 || c > 0x7E || c == 0x22 || c == 0x23 || c == 0x3C || c == 0x3E)
        // TODO: dabar palaiko tik encoding = "UTF-8"; kitų palaikymą galima padaryti pagal:
        // https://cs.chromium.org/chromium/src/url/url_canon_query.cc?rcl=1479817139&l=93
        norm_url_.push_back('?');
        std::size_t norm_len0 = norm_url_.length();
        detail::AppendStringOfType(pointer, end_of_query, detail::CHAR_QUERY, norm_url_); // šis dar koduoja 0x27 '
        set_part(QUERY, norm_len0, norm_url_.length());

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
            CharT ch = *pointer;
            if (ch == 0) {
                // TODO-WARN: Syntax violation
                pointer++;
                continue;
            } else if (static_cast<UCharT>(ch) < 0x80) {
                // Normal ASCII (and control) characters are just appended
                norm_url_.push_back(static_cast<char>(ch));
                pointer++;
            } else {
                // Non-ASCII characters are appended unescaped, but only when they are
                // valid. Invalid Unicode characters are replaced with the "invalid
                // character" as IE seems to (url_util::read_utf_char puts the unicode
                // replacement character in the output on failure for us).
                unsigned code_point;
                url_util::read_utf_char(pointer, last, code_point);
                detail::AppendUTF8Value(code_point, norm_url_);
            }
            // TODO-WARN:
            // If c is not a URL code point and not "%", syntax violation.
            // If c is "%" and remaining does not start with two ASCII hex digits, syntax violation.
        }
        set_part(FRAGMENT, norm_len0, norm_url_.length());
    }

    return true;
}

template <typename CharT>
inline bool url::parse_host(const CharT* first, const CharT* last) {
    //TODO: parse and set host
    return true;
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
                // TODO: 1. If url’s host is non-null, syntax violation.
                // TODO: 2. Set url’s host to null
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
        } else if (uch == '%' && pointer + 2 < last &&
            pointer[1] == '2' && (pointer[2] == 'e' || pointer[2] == 'E'))
        {
            norm_url_ += '.';
            pointer += 3; // skip "%2e"
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

inline void url::append_to_path() {
    if (part_[PATH].len <= 0) {
        part_[PATH].offset = norm_url_.length();
        part_[PATH].len = 0;
    }
    norm_url_.push_back('/');
    part_[PATH].len++;
}

} // namespace whatwg

#endif // WHATWG_URL_H
