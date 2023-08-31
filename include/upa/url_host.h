// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_URL_HOST_H
#define UPA_URL_HOST_H

#include "buffer.h"
#include "str_arg.h"
#include "url_idna.h"
#include "url_ip.h"
#include "url_result.h"
#include "url_utf.h"
#include "util.h"
#include <algorithm> // any_of
#include <cassert>
#include <cstdint> // uint32_t
#include <stdexcept>
#include <string>

namespace upa {

enum class HostType {
    Empty = 0,
    Opaque,
    Domain,
    IPv4,
    IPv6
};


class host_output {
protected:
    host_output() = default;
public:
    host_output(const host_output&) = delete;
    host_output& operator=(const host_output&) = delete;
    virtual ~host_output() = default;

    virtual std::string& hostStart() = 0;
    virtual void hostDone(HostType /*ht*/) = 0;
};

class host_parser {
public:
    template <typename CharT>
    static url_result parse_host(const CharT* first, const CharT* last, bool isNotSpecial, host_output& dest);

    template <typename CharT>
    static url_result parse_opaque_host(const CharT* first, const CharT* last, host_output& dest);

    template <typename CharT>
    static url_result parse_ipv4(const CharT* first, const CharT* last, host_output& dest);

    template <typename CharT>
    static url_result parse_ipv6(const CharT* first, const CharT* last, host_output& dest);
};


// url_host class
// https://github.com/whatwg/url/pull/288
// https://whatpr.org/url/288.html#urlhost-class

class url_host {
public:
    url_host() = delete;
    url_host(const url_host&) = default;
    url_host(url_host&&) noexcept = default;
    url_host& operator=(const url_host&) = default;
    url_host& operator=(url_host&&) UPA_NOEXCEPT_17 = default;

    /// Parsing constructor
    ///
    /// Throws @a url_error exception on parse error.
    ///
    /// @param[in] args Host string to parse
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    explicit url_host(StrT&& str) {
        host_out out(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        const auto res = host_parser::parse_host(inp.begin(), inp.end(), false, out);
        if (res != url_result::Ok)
            throw url_error(res, "Host parse error");
    }

    /// destructor
    ~url_host() = default;

    /// Host type getter
    ///
    /// @return host type, the one of: Domain, IPv4, IPv6
    HostType type() const {
        return type_;
    }

    /// Hostname stringifier
    ///
    /// @return host serialized to string
    std::string to_string() const {
        return host_str_;
    }

private:
    class host_out : public host_output {
    public:
        explicit host_out(url_host& host)
            : host_(host)
        {}
        std::string& hostStart() override {
            return host_.host_str_;
        }
        void hostDone(HostType ht) override {
            host_.type_ = ht;
        }
    private:
        url_host& host_;
    };

    // members
    std::string host_str_;
    HostType type_ = HostType::Empty;
};


// Helper functions

template <typename CharT>
static inline bool contains_forbidden_domain_char(const CharT* first, const CharT* last) {
    return std::any_of(first, last, detail::is_forbidden_domain_char<CharT>);
}

template <typename CharT>
static inline bool contains_forbidden_host_char(const CharT* first, const CharT* last) {
    return std::any_of(first, last, detail::is_forbidden_host_char<CharT>);
}


// The host parser
// https://url.spec.whatwg.org/#concept-host-parser

template <typename CharT>
inline url_result host_parser::parse_host(const CharT* first, const CharT* last, bool isNotSpecial, host_output& dest) {
    using UCharT = typename std::make_unsigned<CharT>::type;

    // 1. Non-"file" special URL's cannot have an empty host.
    // 2. For "file" URL's empty host is set in the file_host_state 1.2
    //    https://url.spec.whatwg.org/#file-host-state
    // 3. Non-special URLs can have empty host and it will be set here.
    if (first == last) {
        // https://github.com/whatwg/url/issues/79
        // https://github.com/whatwg/url/pull/189
        // set empty host
        dest.hostStart();
        dest.hostDone(HostType::Empty);
        return isNotSpecial ? url_result::Ok : url_result::EmptyHost;
    }
    assert(first < last);

    if (*first == '[') {
        if (*(last - 1) == ']') {
            return parse_ipv6(first + 1, last - 1, dest);
        }
        // TODO-ERR: validation error
        return url_result::InvalidIpv6Address;
    }

    if (isNotSpecial)
        return parse_opaque_host(first, last, dest);

    // Is ASCII domain?
    const auto ptr = std::find_if_not(first, last, detail::is_ascii_domain_char<CharT>);
    if (ptr == last) {
        if (!util::has_xn_label(first, last)) {
            // Fast path for ASCII domain

            // If asciiDomain ends in a number, return the result of IPv4 parsing asciiDomain
            if (hostname_ends_in_a_number(first, last))
                return parse_ipv4(first, last, dest);

            // Return asciiDomain lower cased
            std::string& str_host = dest.hostStart();
            util::append_ascii_lowercase(str_host, first, last);
            dest.hostDone(HostType::Domain);
            return url_result::Ok;
        }
    } else if (static_cast<UCharT>(*ptr) < 0x80 && *ptr != '%') {
        return url_result::InvalidDomainCharacter;
    }

    // Input for IDNToASCII
    simple_buffer<char16_t> buff_uc;

    // copy ASCII chars
    for (auto it = first; it != ptr; ++it) {
        const auto uch = static_cast<UCharT>(*it);
        buff_uc.push_back(static_cast<char16_t>(uch));
    }

    // Let buff_uc be the result of running UTF-8 decode (to UTF-16) without BOM
    // on the percent decoding of UTF-8 encode on input
    for (auto it = ptr; it != last;) {
        const auto uch = static_cast<UCharT>(*it++);
        if (uch < 0x80) {
            if (uch != '%') {
                buff_uc.push_back(static_cast<char16_t>(uch));
                continue;
            }
            // uch == '%'
            unsigned char uc8; // NOLINT(cppcoreguidelines-init-variables)
            if (detail::DecodeEscaped(it, last, uc8)) {
                if (uc8 < 0x80) {
                    buff_uc.push_back(static_cast<char16_t>(uc8));
                    continue;
                }
                // percent encoded utf-8 sequence
                // TODO: gal po vieną code_point, tuomet užtektų utf-8 buferio vienam simboliui
                simple_buffer<char> buff_utf8;
                buff_utf8.push_back(static_cast<char>(uc8));
                while (it != last && *it == '%') {
                    ++it; // skip '%'
                    if (!detail::DecodeEscaped(it, last, uc8))
                        uc8 = '%';
                    buff_utf8.push_back(static_cast<char>(uc8));
                }
                url_utf::convert_utf8_to_utf16(buff_utf8.data(), buff_utf8.data() + buff_utf8.size(), buff_uc);
                //buff_utf8.clear();
                continue;
            }
            // detected an invalid percent-encoding sequence
            buff_uc.push_back('%');
        } else { // uch >= 0x80
            --it;
            url_utf::append_utf16(url_utf::read_utf_char(it, last).value, buff_uc);
        }
    }


    // domain to ASCII
    simple_buffer<char16_t> buff_ascii;

    const url_result res = IDNToASCII(buff_uc.data(), buff_uc.size(), buff_ascii);
    if (res != url_result::Ok)
        return res;
    if (contains_forbidden_domain_char(buff_ascii.data(), buff_ascii.data() + buff_ascii.size())) {
        //TODO-ERR: validation error
        return url_result::InvalidDomainCharacter;
    }

    // If asciiDomain ends in a number, return the result of IPv4 parsing asciiDomain
    if (hostname_ends_in_a_number(buff_ascii.begin(), buff_ascii.end()))
        return parse_ipv4(buff_ascii.begin(), buff_ascii.end(), dest);

    // Return asciiDomain
    std::string& str_host = dest.hostStart();
    util::append(str_host, buff_ascii);
    dest.hostDone(HostType::Domain);
    return url_result::Ok;
}

// The opaque-host parser
// https://url.spec.whatwg.org/#concept-opaque-host-parser

template <typename CharT>
inline url_result host_parser::parse_opaque_host(const CharT* first, const CharT* last, host_output& dest) {
    if (contains_forbidden_host_char(first, last))
        return url_result::InvalidDomainCharacter; //TODO-ERR: failure

    // TODO-WARN:
    // 2. If input contains a code point that is not a URL code point and not U+0025 (%), validation error.
    // 3. If input contains a U+0025 (%) and the two code points following it are not ASCII hex digits, validation error.

    std::string& str_host = dest.hostStart();

    //TODO: UTF-8 percent encode it using the C0 control percent-encode set
    //detail::AppendStringOfType(first, last, detail::CHAR_C0_CTRL, str_host);
    using UCharT = typename std::make_unsigned<CharT>::type;

    const CharT* pointer = first;
    while (pointer < last) {
        // UTF-8 percent encode c using the C0 control percent-encode set (U+0000 ... U+001F and >U+007E)
        const auto uch = static_cast<UCharT>(*pointer);
        if (uch >= 0x7f) {
            // invalid utf-8/16/32 sequences will be replaced with 0xfffd
            detail::AppendUTF8EscapedChar(pointer, last, str_host);
        } else {
            // Just append the 7-bit character, escaping C0 control chars:
            if (uch <= 0x1f)
                detail::AppendEscapedChar(uch, str_host);
            else
                str_host.push_back(static_cast<unsigned char>(uch));
            ++pointer;
        }
    }

    dest.hostDone(str_host.empty() ? HostType::Empty : HostType::Opaque);
    return url_result::Ok;
}

template <typename CharT>
inline url_result host_parser::parse_ipv4(const CharT* first, const CharT* last, host_output& dest) {
    uint32_t ipv4;  // NOLINT(cppcoreguidelines-init-variables)

    const url_result res = ipv4_parse(first, last, ipv4);
    if (res == url_result::Ok) {
        std::string& str_ipv4 = dest.hostStart();
        ipv4_serialize(ipv4, str_ipv4);
        dest.hostDone(HostType::IPv4);
    }
    return res;
}

template <typename CharT>
inline url_result host_parser::parse_ipv6(const CharT* first, const CharT* last, host_output& dest) {
    uint16_t ipv6addr[8];

    if (ipv6_parse(first, last, ipv6addr)) {
        std::string& str_ipv6 = dest.hostStart();
        str_ipv6.push_back('[');
        ipv6_serialize(ipv6addr, str_ipv6);
        str_ipv6.push_back(']');
        dest.hostDone(HostType::IPv6);
        return url_result::Ok;
    }
    return url_result::InvalidIpv6Address;
}


} // namespace upa

#endif // UPA_URL_HOST_H
