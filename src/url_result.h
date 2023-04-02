// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef URL_RESULT_H
#define URL_RESULT_H

#include <stdexcept>

namespace whatwg {

/// @brief URL result codes

enum class url_result {
    // Success:
    Ok = 0,                        //!< success 
    False,                         //!< setter ignored the value (internal)
    // Failure:
    InvalidSchemeCharacter,        //!< invalid scheme character
    EmptyHost,                     //!< host cannot be empty
    IdnaError,                     //!< invalid international domain name
    InvalidPort,                   //!< invalid port number
    InvalidIpv4Address,            //!< invalid IPv4 address
    InvalidIpv6Address,            //!< invalid IPv6 address
    InvalidDomainCharacter,        //!< invalid domain character
    RelativeUrlWithoutBase,        //!< relative URL without a base
    RelativeUrlWithCannotBeABase,  //!< relative URL with a cannot-be-a-base base
    InvalidBase,                   //!< invalid base
    Overflow,                      //!< URLs is too long
    // url_from_file_path errors
    EmptyPath,                     //!< path cannot be empty
    UnsupportedPath,               //!< given path unsupported (for example non absolute)
};

/// @brief Check result code indicates success
/// @return `true` if result code is url_result::Ok, `false` otherwise
inline bool success(url_result res) {
    return res == url_result::Ok;
}

/// @brief URL exception class

class url_error : public std::runtime_error {
public:
    /// constructs a new url_error object with the given result code and error message
    ///
    /// @param[in] res result code
    /// @param[in] what_arg error message
    explicit url_error(url_result res, const char* what_arg)
        : std::runtime_error(what_arg)
        , res_(res)
    {}

    /// @return result code
    url_result result() const {
        return res_;
    }
private:
    url_result res_;
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
    operator R() const {
        return result;
    }
};

} // namespace detail
} // namespace whatwg

#endif // URL_RESULT_H
