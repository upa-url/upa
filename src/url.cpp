// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url.h"

namespace upa {
namespace detail {

// url_error exception what() values

const char kURLParseError[] = "URL parse error";
const char kBaseURLParseError[] = "Base URL parse error";

// Part start

const uint8_t kPartStart[url::PART_COUNT] = {
    0, 0, 0,
    1,   // ':' PASSWORD
    0, 0,
    1,   // ':' PORT
    0, 0,
    1,   // '?' QUERY
    1    // '#' FRAGMENT
};

} // namespace detail


  // URL's part flag masks

const unsigned url::kPartFlagMask[url::PART_COUNT] = {
    SCHEME_FLAG,
    0,  // SCHEME_SEP
    USERNAME_FLAG,
    PASSWORD_FLAG,
    0,  // HOST_START
    HOST_FLAG | HOST_TYPE_MASK,
    PORT_FLAG,
    0,  // PATH_PREFIX
    PATH_FLAG | OPAQUE_PATH_FLAG,
    QUERY_FLAG,
    FRAGMENT_FLAG
};

// MUST be sorted by length
const url::scheme_info url::kSchemes[] = {
    // scheme,         port, is_special, is_file, is_http, is_ws
    { { "ws", 2 },       80,          1,       0,       0,     1 },
    { { "wss", 3 },     443,          1,       0,       0,     1 },
    { { "ftp", 3 },      21,          1,       0,       0,     0 },
    { { "http", 4 },     80,          1,       0,       1,     0 },
    { { "file", 4 },     -1,          1,       1,       0,     0 },
    { { "https", 5 },   443,          1,       0,       1,     0 },
};

static const std::size_t max_scheme_length = 5; // "https"

// scheme length to url::kSchemes index
static const uint8_t kLengthToSchemesInd[] = {
    0,  // 0
    0,  // 1
    0,  // 2
    1,  // 3
    3,  // 4
    5,  // 5
    6,  // the end
};

const url::scheme_info* url::get_scheme_info(const str_view_type src) {
    const std::size_t len = src.length();
    if (len <= max_scheme_length) {
        const int end = kLengthToSchemesInd[len + 1];
        for (int ind = kLengthToSchemesInd[len]; ind < end; ++ind) {
            // The src and kSchemes[ind].scheme lengths are the same, so compare data only
            if (str_view_type::traits_type::compare(src.data(), kSchemes[ind].scheme.data(), len) == 0)
                return &kSchemes[ind];
        }
    }
    return nullptr;
}


} // namespace upa
