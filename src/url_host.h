#ifndef WHATWG_URL_HOST_H
#define WHATWG_URL_HOST_H

#include "buffer.h"
#include "url_idna.h"
#include "url_ip.h"
#include "url_result.h"
#include "url_utf.h"
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
    static url_result parse_host(const CharT* first, const CharT* last, bool isNotSpecial, host_output& dest);

    template <typename CharT>
    static url_result parse_opaque_host(const CharT* first, const CharT* last, host_output& dest);

    template <typename CharT>
    static url_result parse_ipv4(const CharT* first, const CharT* last, host_output& dest);

    template <typename CharT>
    static url_result parse_ipv6(const CharT* first, const CharT* last, host_output& dest);
};

template <typename CharT>
static inline bool contains_forbidden_host_char(const CharT* first, const CharT* last) {
    return std::any_of(first, last, detail::isForbiddenHostChar<CharT>);
}

template <typename CharT>
static inline bool contains_forbidden_opaque_host_char(const CharT* first, const CharT* last) {
    return std::any_of(first, last, [](CharT c) {
        return detail::isForbiddenHostChar(c) && c != '%';
    });
}

template <typename CharT>
inline url_result host_parser::parse_host(const CharT* first, const CharT* last, bool isNotSpecial, host_output& dest) {
    using UCharT = typename std::make_unsigned<CharT>::type;

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

    if (isNotSpecial)
        return parse_opaque_host(first, last, dest);


    //TODO: klaidų nustatymas pagal standartą

    // Let buff_uc be the result of running UTF-8 decode (to UTF-16) without BOM
    // on the percent decoding of UTF-8 encode on input
    simple_buffer<char16_t> buff_uc;
    for (auto it = first; it < last;) {
        UCharT uch = static_cast<UCharT>(*it++);
        if (uch < 0x80) {
            if (uch != '%') {
                buff_uc.push_back(static_cast<char16_t>(uch));
                continue;
            }
            // uch == '%'
            unsigned char uc8;
            if (detail::DecodeEscaped(it, last, uc8)) {
                if (uc8 < 0x80) {
                    buff_uc.push_back(static_cast<char16_t>(uc8));
                    continue;
                }
                // percent encoded utf-8 sequence
                // TODO: gal po vieną code_point, tuomet užtektų utf-8 buferio vienam simboliui
                simple_buffer<unsigned char> buff_utf8;
                buff_utf8.push_back(uc8);
                while (it < last && *it == '%') {
                    it++; // skip '%'
                    if (!detail::DecodeEscaped(it, last, uc8))
                        uc8 = '%';
                    buff_utf8.push_back(uc8);
                }
                url_utf::convert_utf8_to_utf16(buff_utf8.data(), buff_utf8.data() + buff_utf8.size(), buff_uc);
                //buff_utf8.clear();
                continue;
            }
            // detected invalid percent encode
            buff_uc.push_back('%');
        } else { // uch >= 0x80
            uint32_t code_point;
            it--;
            url_utf::read_utf_char(it, last, code_point);
            url_utf::append_utf16(code_point, buff_uc);
        }
    }


    // domain to ASCII
    simple_buffer<char16_t> buff_ascii;

    url_result res = IDNToASCII(buff_uc.data(), buff_uc.size(), buff_ascii);
    if (res != url_result::Ok)
        return res;
    if (contains_forbidden_host_char(buff_ascii.data(), buff_ascii.data() + buff_ascii.size())) {
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
    if (contains_forbidden_opaque_host_char(first, last))
        return url_result::InvalidDomainCharacter; //TODO-ERR: failure

    std::string& str_host = dest.hostStart();

    //TODO: UTF-8 percent encode it using the C0 control percent-encode set
    //detail::AppendStringOfType(first, last, detail::CHAR_C0_CTRL, str_host);
    using UCharT = typename std::make_unsigned<CharT>::type;

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
