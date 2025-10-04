// Copyright 2016-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_URL_HOST_H
#define UPA_URL_HOST_H

#include "buffer.h"
#include "config.h"
#include "idna.h"
#include "str_arg.h"
#include "url_ip.h"
#include "url_percent_encode.h"
#include "url_result.h"
#include "url_utf.h"
#include "util.h"
#include <algorithm> // any_of
#include <cassert>
#include <cstdint> // uint16_t, uint32_t
#include <string>
#include <type_traits>

namespace upa {

/// @brief Host representation
///
/// See: https://url.spec.whatwg.org/#host-representation
enum class HostType {
    Empty = 0, ///< **empty host** is the empty string
    Opaque,    ///< **opaque host** is a non-empty ASCII string used in a not special URL
    Domain,    ///< **domain** is a non-empty ASCII string that identifies a realm within a network
               ///< (it is usually the host of a special URL)
    IPv4,      ///< host is an **IPv4 address**
    IPv6       ///< host is an **IPv6 address**
};


class host_output {
protected:
    host_output() = default;
    host_output(bool need_save)
        : need_save_{ need_save } {}
public:
    host_output(const host_output&) = delete;
    host_output& operator=(const host_output&) = delete;
    virtual ~host_output() = default;

    virtual std::string& hostStart() = 0;
    virtual void hostDone(HostType /*ht*/) = 0;
    bool need_save() const noexcept { return need_save_; }
private:
    bool need_save_ = true;
};

class host_parser {
public:
    template <typename CharT>
    static validation_errc parse_host(const CharT* first, const CharT* last, bool is_opaque, host_output& dest);

    template <typename CharT>
    static validation_errc parse_opaque_host(const CharT* first, const CharT* last, host_output& dest);

    template <typename CharT>
    static validation_errc parse_ipv4(const CharT* first, const CharT* last, host_output& dest);

    template <typename CharT>
    static validation_errc parse_ipv6(const CharT* first, const CharT* last, host_output& dest);
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
    url_host& operator=(url_host&&) noexcept = default;

    /// Parsing constructor
    ///
    /// Throws @a url_error exception on parse error.
    ///
    /// @param[in] str Host string to parse
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    explicit url_host(StrT&& str) {
        host_out out(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        const auto res = host_parser::parse_host(inp.begin(), inp.end(), false, out);
        if (res != validation_errc::ok)
            throw url_error(res, "Host parse error");
    }

    /// destructor
    ~url_host() = default;

    /// Host type getter
    ///
    /// @return host type, the one of: Domain, IPv4, IPv6
    [[nodiscard]] HostType type() const {
        return type_;
    }

    /// Hostname view
    ///
    /// @return serialized host as string_view
    [[nodiscard]] string_view name() const UPA_LIFETIMEBOUND {
        return host_str_;
    }

    /// Hostname stringifier
    ///
    /// @return host serialized to string
    [[nodiscard]] std::string to_string() const {
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
        url_host& host_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    };

    // members
    std::string host_str_;
    HostType type_ = HostType::Empty;
};


// Helper functions

namespace detail {

template <typename CharT>
inline bool contains_forbidden_domain_char(const CharT* first, const CharT* last) {
    return std::any_of(first, last, detail::is_forbidden_domain_char<CharT>);
}

template <typename CharT>
inline bool contains_forbidden_host_char(const CharT* first, const CharT* last) {
    return std::any_of(first, last, detail::is_forbidden_host_char<CharT>);
}

} // namespace detail


// IDNA
// https://url.spec.whatwg.org/#idna

/// @brief Implements the domain to Unicode algorithm
///
/// See: https://url.spec.whatwg.org/#concept-domain-to-unicode
/// The domain to Unicode result is appended to the @a output, even if the
/// function returns `false`.
///
/// @param[out] output string to store result
/// @param[in]  input source domain string
/// @param[in]  be_strict
/// @param[in]  is_input_ascii
/// @return `true` on success, or `false` on errors
template <class CharT, class StrT, enable_if_str_arg_t<StrT> = 0>
inline bool domain_to_unicode(std::basic_string<CharT>& output, StrT&& input,
    bool be_strict = false, bool is_input_ascii = false)
{
    const auto inp = make_str_arg(std::forward<StrT>(input));
    if constexpr (std::is_same_v<CharT, char32_t>) {
        return idna::domain_to_unicode(output, inp.begin(), inp.end(), be_strict, is_input_ascii);
    } else {
        std::u32string domain;
        const bool res = idna::domain_to_unicode(domain, inp.begin(), inp.end(), be_strict, is_input_ascii);
        if constexpr (sizeof(CharT) == sizeof(char)) {
            // CharT is char8_t, or char
            for (auto cp : domain)
                url_utf::append_utf8<std::basic_string<CharT>, detail::append_to_string>(cp, output);
        } else if constexpr (sizeof(CharT) == sizeof(char16_t)) {
            // CharT is char16_t, or wchar_t (Windows)
            for (auto cp : domain)
                url_utf::append_utf16(cp, output);
        } else if constexpr (sizeof(CharT) == sizeof(char32_t)) {
            // CharT is wchar_t (non Windows)
            util::append(output, domain);
        } else {
            static_assert(util::false_v<CharT>, "unsupported output character type");
        }
        return res;
    }
}

// The host parser
// https://url.spec.whatwg.org/#concept-host-parser

template <typename CharT>
inline validation_errc host_parser::parse_host(const CharT* first, const CharT* last, bool is_opaque, host_output& dest) {
    using UCharT = std::make_unsigned_t<CharT>;

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
        return is_opaque ? validation_errc::ok : validation_errc::host_missing;
    }
    assert(first < last);

    if (*first == '[') {
        if (*(last - 1) == ']') {
            return parse_ipv6(first + 1, last - 1, dest);
        }
        // 1.1. If input does not end with U+005D (]), IPv6-unclosed
        // validation error, return failure.
        return validation_errc::ipv6_unclosed;
    }

    if (is_opaque)
        return parse_opaque_host(first, last, dest);

    // Is ASCII domain?
    const auto ptr = std::find_if_not(first, last, detail::is_ascii_domain_char<CharT>);
    if (ptr == last) {
        if (!util::has_xn_label(first, last)) {
            // Fast path for ASCII domain

            // If asciiDomain ends in a number, return the result of IPv4 parsing asciiDomain
            if (hostname_ends_in_a_number(first, last))
                return parse_ipv4(first, last, dest);

            if (dest.need_save()) {
                // Return asciiDomain lower cased
                std::string& str_host = dest.hostStart();
                util::append_ascii_lowercase(str_host, first, last);
                dest.hostDone(HostType::Domain);
            }
            return validation_errc::ok;
        }
    } else if (static_cast<UCharT>(*ptr) < 0x80 && *ptr != '%') {
        // NFC normalizes U+003C (<), U+003D (=), U+003E (>) characters if they precede
        // U+0338. Therefore, no errors are reported here for forbidden < and > characters
        // if there is a possibility to normalize them.
        if (!(*ptr >= 0x3C && *ptr <= 0x3E && ptr + 1 < last && static_cast<UCharT>(ptr[1]) >= 0x80))
            // 7. If asciiDomain contains a forbidden domain code point, domain-invalid-code-point
            // validation error, return failure.
            return validation_errc::domain_invalid_code_point;
    }

    std::string buff_ascii;

    const auto pes = std::find(ptr, last, '%');
    if (pes == last) {
        // Input is ASCII if ptr == last
        if (!idna::domain_to_ascii(buff_ascii, first, last, false, ptr == last))
            return validation_errc::domain_to_ascii;
    } else {
        // Input for domain_to_ascii
        simple_buffer<char32_t> buff_uc;

        // copy ASCII chars
        for (auto it = first; it != ptr; ++it) {
            const auto uch = static_cast<UCharT>(*it);
            buff_uc.push_back(static_cast<char32_t>(uch));
        }

        // Let buff_uc be the result of running UTF-8 decode (to UTF-16) without BOM
        // on the percent decoding of UTF-8 encode on input
        for (auto it = ptr; it != last;) {
            const auto uch = static_cast<UCharT>(*it++);
            if (uch < 0x80) {
                if (uch != '%') {
                    buff_uc.push_back(static_cast<char32_t>(uch));
                    continue;
                }
                // uch == '%'
                unsigned char uc8; // NOLINT(cppcoreguidelines-init-variables)
                if (detail::decode_hex_to_byte(it, last, uc8)) {
                    if (uc8 < 0x80) {
                        buff_uc.push_back(static_cast<char32_t>(uc8));
                        continue;
                    }
                    // percent encoded utf-8 sequence
                    // TODO: gal po vieną code_point, tuomet užtektų utf-8 buferio vienam simboliui
                    simple_buffer<char> buff_utf8;
                    buff_utf8.push_back(static_cast<char>(uc8));
                    while (it != last && *it == '%') {
                        ++it; // skip '%'
                        if (!detail::decode_hex_to_byte(it, last, uc8))
                            uc8 = '%';
                        buff_utf8.push_back(static_cast<char>(uc8));
                    }
                    // utf-8 to utf-32
                    const auto* last_utf8 = buff_utf8.data() + buff_utf8.size();
                    for (const auto* it_utf8 = buff_utf8.data(); it_utf8 < last_utf8;)
                        buff_uc.push_back(url_utf::read_utf_char(it_utf8, last_utf8).value);
                    //buff_utf8.clear();
                    continue;
                }
                // detected an invalid percent-encoding sequence
                buff_uc.push_back('%');
            } else { // uch >= 0x80
                --it;
                buff_uc.push_back(url_utf::read_utf_char(it, last).value);
            }
        }
        if (!idna::domain_to_ascii(buff_ascii, buff_uc.begin(), buff_uc.end()))
            return validation_errc::domain_to_ascii;
    }

    if (detail::contains_forbidden_domain_char(buff_ascii.data(), buff_ascii.data() + buff_ascii.size())) {
        // 7. If asciiDomain contains a forbidden domain code point, domain-invalid-code-point
        // validation error, return failure.
        return validation_errc::domain_invalid_code_point;
    }

    // If asciiDomain ends in a number, return the result of IPv4 parsing asciiDomain
    if (hostname_ends_in_a_number(buff_ascii.data(), buff_ascii.data() + buff_ascii.size()))
        return parse_ipv4(buff_ascii.data(), buff_ascii.data() + buff_ascii.size(), dest);

    if (dest.need_save()) {
        // Return asciiDomain
        std::string& str_host = dest.hostStart();
        str_host.append(buff_ascii);
        dest.hostDone(HostType::Domain);
    }
    return validation_errc::ok;
}

// The opaque-host parser
// https://url.spec.whatwg.org/#concept-opaque-host-parser

template <typename CharT>
inline validation_errc host_parser::parse_opaque_host(const CharT* first, const CharT* last, host_output& dest) {
    // 1. If input contains a forbidden host code point, host-invalid-code-point
    // validation error, return failure.
    if (detail::contains_forbidden_host_char(first, last))
        return validation_errc::host_invalid_code_point;

    // TODO-WARN:
    // 2. If input contains a code point that is not a URL code point and not U+0025 (%),
    // invalid-URL-unit validation error.
    // 3. If input contains a U+0025 (%) and the two code points following it are not ASCII hex digits,
    // invalid-URL-unit validation error.

    if (dest.need_save()) {
        std::string& str_host = dest.hostStart();

        //TODO: UTF-8 percent encode it using the C0 control percent-encode set
        //detail::append_utf8_percent_encoded(first, last, detail::CHAR_C0_CTRL, str_host);
        using UCharT = std::make_unsigned_t<CharT>;

        const CharT* pointer = first;
        while (pointer < last) {
            // UTF-8 percent encode c using the C0 control percent-encode set (U+0000 ... U+001F and >U+007E)
            const auto uch = static_cast<UCharT>(*pointer);
            if (uch >= 0x7f) {
                // invalid utf-8/16/32 sequences will be replaced with 0xfffd
                detail::append_utf8_percent_encoded_char(pointer, last, str_host);
            } else {
                // Just append the 7-bit character, percent encoding C0 control chars
                const auto uc = static_cast<unsigned char>(uch);
                if (uc <= 0x1f)
                    detail::append_percent_encoded_byte(uc, str_host);
                else
                    str_host.push_back(uc);
                ++pointer;
            }
        }

        dest.hostDone(str_host.empty() ? HostType::Empty : HostType::Opaque);
    }
    return validation_errc::ok;
}

template <typename CharT>
inline validation_errc host_parser::parse_ipv4(const CharT* first, const CharT* last, host_output& dest) {
    uint32_t ipv4;  // NOLINT(cppcoreguidelines-init-variables)

    const auto res = ipv4_parse(first, last, ipv4);
    if (res == validation_errc::ok && dest.need_save()) {
        std::string& str_ipv4 = dest.hostStart();
        ipv4_serialize(ipv4, str_ipv4);
        dest.hostDone(HostType::IPv4);
    }
    return res;
}

template <typename CharT>
inline validation_errc host_parser::parse_ipv6(const CharT* first, const CharT* last, host_output& dest) {
    uint16_t ipv6addr[8];

    const auto res = ipv6_parse(first, last, ipv6addr);
    if (res == validation_errc::ok && dest.need_save()) {
        std::string& str_ipv6 = dest.hostStart();
        str_ipv6.push_back('[');
        ipv6_serialize(ipv6addr, str_ipv6);
        str_ipv6.push_back(']');
        dest.hostDone(HostType::IPv6);
    }
    return res;
}


} // namespace upa

#endif // UPA_URL_HOST_H
