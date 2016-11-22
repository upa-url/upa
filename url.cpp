
#include "url.hpp"
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


// MUST by sorted alphabetically by scheme
const scheme_info kSchemes[] = {
    // scheme,            port, is_special, is_local, is_http, is_network, is_fetch, is_file, is_ws
    { { "about", 5 },        0,          0,        1,       0,          0,        1,       0,     0 },
    { { "blob", 4 },         0,          0,        1,       0,          0,        1,       0,     0 },
    { { "data", 4 },         0,          0,        1,       0,          0,        1,       0,     0 },
    { { "file", 4 },         0,          1,        0,       0,          0,        1,       1,     0 },
    { { "filesystem", 10 },  0,          0,        1,       0,          0,        1,       0,     0 },
    { { "ftp", 3 },         21,          1,        0,       0,          1,        1,       0,     0 },
    { { "gopher", 6 },      70,          1,        0,       0,          0,        0,       0,     0 },
    { { "http", 4 },        80,          1,        0,       1,          1,        1,       0,     0 },
    { { "https", 5 },      443,          1,        0,       1,          1,        1,       0,     0 },
    { { "ws", 2 },          80,          1,        0,       0,          0,        0,       0,     1 },
    { { "wss", 3 },        443,          1,        0,       0,          0,        0,       0,     1 }
};

const scheme_info* get_scheme_info(const str_view<char> src) {
    auto it = std::lower_bound(std::begin(kSchemes), std::end(kSchemes), src,
        [](const str_view<char>& a, const str_view<char>& b) { return a.compare(b) < 0; });
    if (it != std::end(kSchemes) && it->scheme.equal(src))
        return it;
    return nullptr;
}


} // namespace detail
} // namespace whatwg
