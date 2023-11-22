// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_URL_RESULT_H
#define UPA_URL_RESULT_H

#include <stdexcept>

namespace upa {

/// @brief URL validation and other error codes
///
/// See: https://url.spec.whatwg.org/#validation-error
enum class validation_errc {
    // Success:
    ok = 0,                         ///< success
    // Ignored input (for internal use):
    ignored,                        ///< setter ignored the value (internal)
    scheme_invalid_code_point,      ///< the scheme contains invalid code point (internal,
                                    ///< relevant to the protocol setter)

    // Standard validation error codes
    // https://url.spec.whatwg.org/#validation-error

    // Non-failure (only ipv4_out_of_range_part indicates failure in some cases,
    // see: https://url.spec.whatwg.org/#ipv4-out-of-range-part):
    // IDNA
    domain_to_unicode,              ///< Unicode ToUnicode records an error
    // Host parsing
    ipv4_empty_part,                ///< An IPv4 address ends with a U+002E (.)
    ipv4_non_decimal_part,          ///< The IPv4 address contains numbers expressed using hexadecimal or octal digits
    ipv4_out_of_range_part,         ///< An IPv4 address part exceeds 255
    // URL parsing
    invalid_url_unit,               ///< A code point is found that is not a URL unit
    special_scheme_missing_following_solidus, ///< The input’s scheme is not followed by "//"
    invalid_reverse_solidus,        ///< The URL has a special scheme and it uses U+005C (\) instead of U+002F (/)
    invalid_credentials,            ///< The input includes credentials
    file_invalid_windows_drive_letter, ///< The input is a relative-URL string that starts with a Windows drive letter
                                       ///< and the base URL’s scheme is "file"
    file_invalid_windows_drive_letter_host, ///< A file: URL’s host is a Windows drive letter

    // Failure:
    // IDNA
    domain_to_ascii,                ///< Unicode ToASCII records an error or returns the empty string
    // Host parsing
    domain_invalid_code_point,      ///< The input’s host contains a forbidden domain code point
    host_invalid_code_point,        ///< An opaque host contains a forbidden host code point
    ipv4_too_many_parts,            ///< An IPv4 address does not consist of exactly 4 parts
    ipv4_non_numeric_part,          ///< An IPv4 address part is not numeric
    ipv6_unclosed,                  ///< An IPv6 address is missing the closing U+005D (])
    ipv6_invalid_compression,       ///< An IPv6 address begins with improper compression
    ipv6_too_many_pieces,           ///< An IPv6 address contains more than 8 pieces
    ipv6_multiple_compression,      ///< An IPv6 address is compressed in more than one spot
    ipv6_invalid_code_point,        ///< An IPv6 address contains a code point that is neither an ASCII hex digit nor
                                    ///< a U+003A (:). Or it unexpectedly ends
    ipv6_too_few_pieces,            ///< An uncompressed IPv6 address contains fewer than 8 pieces
    ipv4_in_ipv6_too_many_pieces,   ///< An IPv6 address with IPv4 address syntax: the IPv6 address has more than 6 pieces
    ipv4_in_ipv6_invalid_code_point, ///< An IPv6 address with IPv4 address syntax:
                                     ///< * An IPv4 part is empty or contains a non-ASCII digit
                                     ///< * An IPv4 part contains a leading 0
                                     ///< * There are too many IPv4 parts
    ipv4_in_ipv6_out_of_range_part, ///< An IPv6 address with IPv4 address syntax: an IPv4 part exceeds 255
    ipv4_in_ipv6_too_few_parts,     ///< An IPv6 address with IPv4 address syntax: an IPv4 address contains too few parts
    // URL parsing
    missing_scheme_non_relative_url, ///< The input is missing a scheme, because it does not begin with an ASCII alpha, and
                                     ///< either no base URL was provided or the base URL cannot be used as a base URL
                                     ///< because it has an opaque path
    host_missing,                   ///< The input has a special scheme, but does not contain a host
    port_out_of_range,              ///< The input’s port is too big
    port_invalid,                   ///< The input’s port is invalid

    // Non-standard error codes (indicates failure)

    overflow,                       ///< URL is too long
    invalid_base,                   ///< Invalid base
    // url_from_file_path errors
    file_empty_path,                ///< Path cannot be empty
    file_unsupported_path,          ///< Unsupported file path (e.g. non-absolute)
    // path_from_file_url errors
    not_file_url,                   ///< Not a file URL
    file_url_cannot_have_host,      ///< POSIX path cannot have host
    file_url_invalid_unc,           ///< Invalid UNC path in file URL
    file_url_not_windows_path,      ///< Not a Windows path in file URL
};

/// @brief Check validation error code indicates success
/// @return `true` if validation error code is validation_errc::ok, `false` otherwise
constexpr bool success(validation_errc res) noexcept {
    return res == validation_errc::ok;
}

/// @brief URL exception class

class url_error : public std::runtime_error {
public:
    /// constructs a new url_error object with the given result code and error message
    ///
    /// @param[in] res validation error code
    /// @param[in] what_arg error message
    explicit url_error(validation_errc res, const char* what_arg)
        : std::runtime_error(what_arg)
        , res_(res)
    {}

    /// @return validation error code
    validation_errc result() const noexcept {
        return res_;
    }
private:
    validation_errc res_;
};

namespace detail {

/// @brief Result/value pair

template<typename T, typename R = bool>
struct result_value {
    T value{};
    R result{};

    result_value(R res) noexcept
        : result(res) {}
    result_value(R res, T val) noexcept
        : value(val), result(res) {}
    operator R() const noexcept {
        return result;
    }
};

} // namespace detail
} // namespace upa

#endif // UPA_URL_RESULT_H
