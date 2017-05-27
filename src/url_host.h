#ifndef WHATWG_URL_HOST_H
#define WHATWG_URL_HOST_H

#include "buffer.h"
#include "url_canon.h"
#include "url_idna.h"
#include "url_ip.h"
#include "url_result.h"
#include <cassert>
#include <cstdint> // uint32_t
#include <string>

namespace whatwg {

enum class HostType {
    Empty = 0,
    Opaque,
    Domain,
    IPv4,
    IPv6
};

//union ip_address {
//  uint32_t ipv4;
//  //TODO
//};

class host_output {
public:
    virtual std::string& hostStart() = 0;
    virtual void hostDone(HostType ht) {}
};

class host_parser {
public:
    template <typename CharT>
    static url_result parse_host(const CharT* first, const CharT* last, bool isSpecial, host_output& dest);

    template <typename CharT>
    static url_result parse_opaque_host(const CharT* first, const CharT* last, host_output& dest);

    template <typename CharT>
    static url_result parse_ipv4(const CharT* first, const CharT* last, host_output& dest);

    template <typename CharT>
    static url_result parse_ipv6(const CharT* first, const CharT* last, host_output& dest);
};

template <typename CharT>
static inline bool is_valid_host_chars(const CharT* first, const CharT* last) {
    return std::none_of(first, last, detail::IsInvalidHostChar<CharT>);
}

template <typename CharT>
static inline bool is_valid_opaque_host_chars(const CharT* first, const CharT* last) {
    return std::none_of(first, last, [](CharT c) {
        return detail::IsInvalidHostChar(c) && c != '%';
    });
}

template <typename CharT>
inline url_result host_parser::parse_host(const CharT* first, const CharT* last, bool isSpecial, host_output& dest) {
    typedef std::make_unsigned<CharT>::type UCharT;

    // 1. Non-"file" special URL's cannot have an empty host.
    // 2. For "file" URL's empty host is set in the file_host_state 1.2
    //    https://url.spec.whatwg.org/#file-host-state
    // So empty host here will be set only for non-special URL's, instead of in
    // the opaque host parser (if follow URL standard).
    if (first == last) {
        // https://github.com/whatwg/url/issues/79
        // https://github.com/whatwg/url/pull/189
        // set empty host
        dest.hostStart();
        dest.hostDone(HostType::Empty);
        return url_result::Ok;
    }
    assert(first < last);

    if (*first == '[') {
        if (*(last - 1) == ']') {
            return parse_ipv6(first + 1, last - 1, dest);
        } else {
            // TODO-ERR: validation error
            return url_result::InvalidIpv6Address;
        }
    }

    if (!isSpecial)
        return parse_opaque_host(first, last, dest);

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

    //TODO: klaid� nustatymas pagal standart�

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
        // Net ir ASCII turi praeiti IDNToASCII patikrinim�;
        // ta�iau �r.: https://github.com/jsdom/whatwg-url/issues/50
        //  ^-- kad korekti�kai veikt� reikia Unicode 9 palaikymo
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

    url_result res = IDNToASCII(buff_uc.data(), buff_uc.length(), buff_ascii);
    if (res != url_result::Ok)
        return res;
    if (!is_valid_host_chars(buff_ascii.data(), buff_ascii.data() + buff_ascii.size())) {
        //TODO-ERR: validation error
        return url_result::InvalidDomainCharacter;
    }
    // IPv4
    res = parse_ipv4(buff_ascii.begin(), buff_ascii.end(), dest);
    if (res == url_result::False) {
        std::string& str_host = dest.hostStart();
        str_host.append(buff_ascii.begin(), buff_ascii.end());
        dest.hostDone(HostType::Domain);
        return url_result::Ok;
    }
    return res;
}

template <typename CharT>
inline url_result host_parser::parse_opaque_host(const CharT* first, const CharT* last, host_output& dest) {
    if (!is_valid_opaque_host_chars(first, last))
        return url_result::InvalidDomainCharacter; //TODO-ERR: failure

    std::string& str_host = dest.hostStart();

    //TODO: UTF-8 percent encode it using the C0 control percent-encode set
    //detail::AppendStringOfType(first, last, detail::CHAR_C0_CTRL, str_host);
    typedef std::make_unsigned<CharT>::type UCharT;
    const CharT* pointer = first;
    while (pointer < last) {
        // UTF-8 percent encode c using the C0 control percent-encode set (U+0000 ... U+001F and >U+007E)
        UCharT uch = static_cast<UCharT>(*pointer);
        if (uch >= 0x7f) {
            // invalid utf-8/16/32 sequences will be replaced with 0xfffd
            detail::AppendUTF8EscapedChar(pointer, last, str_host);
        } else {
            // Just append the 7-bit character, escaping C0 control chars:
            if (uch <= 0x1f)
                detail::AppendEscapedChar(uch, str_host);
            else
                str_host.push_back(static_cast<unsigned char>(uch));
            pointer++;
        }
    }

    dest.hostDone(str_host.empty() ? HostType::Empty : HostType::Opaque);
    return url_result::Ok;
}

template <typename CharT>
inline url_result host_parser::parse_ipv4(const CharT* first, const CharT* last, host_output& dest) {
    uint32_t ipv4;

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


} // namespace whatwg

#endif // WHATWG_URL_HOST_H