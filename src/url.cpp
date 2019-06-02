// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
// This file contains portions of modified code from:
// https://cs.chromium.org/chromium/src/url/url_canon_etc.cc
// Copyright 2013 The Chromium Authors. All rights reserved.
//

#include "url.h"
#include <algorithm>

namespace whatwg {
namespace detail {

// SCHEME

// https://cs.chromium.org/chromium/src/url/url_canon_etc.cc

// Contains the canonical version of each possible input letter in the scheme
// (basically, lower-cased). The corresponding entry will be 0 if the letter
// is not allowed in a scheme.
const char kSchemeCanonical[0x80] = {
// 00-1f: all are invalid
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
//  ' '   !    "    #    $    %    &    '    (    )    *    +    ,    -    .    /
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  '+',  0,  '-', '.',  0,
//   0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
//   @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O
     0 , 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
//   P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',  0,   0 ,  0,   0 ,  0,
//   `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o
     0 , 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
//   p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',  0 ,  0 ,  0 ,  0 ,  0
};

// Part start

const uint8_t kPartStart[url::PART_COUNT] = {
    0, 0, 0,
    1,   // ':' PASSWORD
    0, 0,
    1,   // ':' PORT
    0,
    1,   // '?' QUERY
    1    // '#' FRAGMENT
};

} // namespace detail


// MUST be sorted by length
const url::scheme_info url::kSchemes[] = {
    // scheme,         port, is_special, is_file, is_ws
    { { "ws", 2 },       80,          1,       0,     1 },
    { { "wss", 3 },     443,          1,       0,     1 },
    { { "ftp", 3 },      21,          1,       0,     0 },
    { { "http", 4 },     80,          1,       0,     0 },
    { { "file", 4 },     -1,          1,       1,     0 },
    { { "https", 5 },   443,          1,       0,     0 },
    { { "gopher", 6 },   70,          1,       0,     0 },
};

// scheme length to url::kSchemes index
static const unsigned char kLengthToSchemesInd[] = {
    0,  // 0
    0,  // 1
    0,  // 2
    1,  // 3
    3,  // 4
    5,  // 5
    6,  // 6
    7,  // the end
};

const url::scheme_info* url::get_scheme_info(const str_view_type src) {
    const std::size_t len = src.length();
    // max scheme length = 6 ("gopher")
    if (len <= 6) {
        const int end = kLengthToSchemesInd[len + 1];
        for (int ind = kLengthToSchemesInd[len]; ind < end; ind++) {
            // src == kSchemes[ind].scheme, but length is the same and equal to the len
            if (str_view_type::traits_type::compare(src.data(), kSchemes[ind].scheme.data(), len) == 0)
                return &kSchemes[ind];
        }
    }
    return nullptr;
}


} // namespace whatwg
