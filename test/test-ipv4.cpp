// Copyright 2016-2021 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url.h"
#include "doctest-main.h"


static whatwg::url_result ipv4_parse(const char* szInput, uint32_t& ipv4) {
    using str = std::char_traits<char>;
    return whatwg::ipv4_parse(szInput, szInput + str::length(szInput), ipv4);
};

TEST_CASE("IPv4 parser test with empty input") {
    const char* szEmpty = "";

    // https://url.spec.whatwg.org/#ipv4-number-parser
    // 4. If input is the empty string, then return zero.
    uint32_t ipv4 = 1;
    CHECK(whatwg::ipv4_parse_number(szEmpty, szEmpty, ipv4) == whatwg::url_result::Ok);
    CHECK(ipv4 == 0);

    // https://url.spec.whatwg.org/#concept-ipv4-parser
    // 6. 1. If part is the empty string, return input.
    CHECK(whatwg::ipv4_parse(szEmpty, szEmpty, ipv4) == whatwg::url_result::False);
}

TEST_CASE("IPv4 parser test with 127.0.0.1") {
    uint32_t ipv4;

    ipv4 = 0;
    CHECK(ipv4_parse("0x7f000001", ipv4) == whatwg::url_result::Ok);
    CHECK(ipv4 == 0x7f000001);

    ipv4 = 0;
    CHECK(ipv4_parse("0x7f.0.0.1", ipv4) == whatwg::url_result::Ok);
    CHECK(ipv4 == 0x7f000001);

    ipv4 = 0;
    CHECK(ipv4_parse("127.0.0.1", ipv4) == whatwg::url_result::Ok);
    CHECK(ipv4 == 0x7f000001);

    ipv4 = 0;
    CHECK(ipv4_parse("127.0.1", ipv4) == whatwg::url_result::Ok);
    CHECK(ipv4 == 0x7f000001);

    ipv4 = 0;
    CHECK(ipv4_parse("127.1", ipv4) == whatwg::url_result::Ok);
    CHECK(ipv4 == 0x7f000001);
}

TEST_CASE("127.0.0.1 percent encoded") {
    whatwg::url url;

    // 127.0.0.1 percent encoded
    CHECK(whatwg::success(url.parse("http://12%37.0.0.1/", nullptr)));
    CHECK(url.hostname() == "127.0.0.1");

    // 0x7f.0.0.1 percent encoded
    CHECK(whatwg::success(url.parse("http://%30%78%37%66.0.0.1/", nullptr)));
    CHECK(url.hostname() == "127.0.0.1");
}
