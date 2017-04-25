#ifndef URL_RESULT_H
#define URL_RESULT_H

namespace whatwg {

enum class url_result {
    // success
    Ok = 0,
    False,                         // internal
    // failure
    EmptyHost,                     // host cannot be empty
    IdnaError,                     // invalid international domain name
    InvalidPort,                   // invalid port number
    InvalidIpv4Address,            // invalid IPv4 address
    InvalidIpv6Address,            // invalid IPv6 address
    InvalidDomainCharacter,        // invalid domain character
    RelativeUrlWithoutBase,        // relative URL without a base
    RelativeUrlWithCannotBeABase,  // relative URL with a cannot-be-a-base base
    SetHostOnCannotBeABaseUrl,     // a cannot-be-a-base URL doesn’t have a host to set
    Overflow,                      // URLs is too long
};

}

#endif // URL_RESULT_H
