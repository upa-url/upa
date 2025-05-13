// Copyright 2016-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url.h"

namespace upa {
namespace detail {

// Scheme info

namespace {

// MUST be sorted by length
const scheme_info kSchemes[] = {
    // scheme,         port, is_special, is_file, is_http, is_ws
    { { "ws", 2 },       80,          1,       0,       0,     1 },
    { { "wss", 3 },     443,          1,       0,       0,     1 },
    { { "ftp", 3 },      21,          1,       0,       0,     0 },
    { { "http", 4 },     80,          1,       0,       1,     0 },
    { { "file", 4 },     -1,          1,       1,       0,     0 },
    { { "https", 5 },   443,          1,       0,       1,     0 },
};

const std::size_t max_scheme_length = 5; // "https"

// scheme length to url::kSchemes index
const uint8_t kLengthToSchemesInd[] = {
    0,  // 0
    0,  // 1
    0,  // 2
    1,  // 3
    3,  // 4
    5,  // 5
    6,  // the end
};

} // namespace

const scheme_info* get_scheme_info(const string_view src) {
    const std::size_t len = src.length();
    if (len <= max_scheme_length) {
        const int end = kLengthToSchemesInd[len + 1];
        for (int ind = kLengthToSchemesInd[len]; ind < end; ++ind) {
            // The src and kSchemes[ind].scheme lengths are the same, so compare data only
            if (string_view::traits_type::compare(src.data(), kSchemes[ind].scheme.data(), len) == 0)
                return &kSchemes[ind];
        }
    }
    return nullptr;
}

// url_error exception what() values

const char* const kURLParseError = "URL parse error";
const char* const kBaseURLParseError = "Base URL parse error";

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

// Gets start and end position of the specified URL part

std::pair<std::size_t, std::size_t> url::get_part_pos(PartType t, bool with_sep) const {
    if (t == SCHEME)
        return { 0, part_end_[SCHEME] + static_cast<std::size_t>(with_sep &&
            part_end_[SCHEME] != 0) };
    const std::size_t e = part_end_[t];
    if (e) {
        std::size_t b = part_end_[t - 1];
        if (b < e && (!with_sep || t < QUERY || e - b == 1))
            b += detail::kPartStart[t];
        return { b, e };
    }
    return { norm_url_.size(), norm_url_.size() };
}

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

// Upa URL version encoded to one number

std::uint32_t version_num() {
    return UPA_URL_VERSION_NUM;
}


} // namespace upa
