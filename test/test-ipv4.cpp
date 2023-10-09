// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url.h"
#include "doctest-main.h"


static upa::validation_errc ipv4_parse(const char* szInput, uint32_t& ipv4) {
    using str = std::char_traits<char>;
    return upa::ipv4_parse(szInput, szInput + str::length(szInput), ipv4);
};

TEST_CASE("IPv4 parser test with empty input") {
    const char* szEmpty = "";

    // https://url.spec.whatwg.org/#ipv4-number-parser
    // 1. If input is the empty string, then return failure.
    uint32_t ipv4 = 1;
    CHECK(upa::ipv4_parse_number(szEmpty, szEmpty, ipv4) != upa::validation_errc::ok);

    // https://url.spec.whatwg.org/#concept-ipv4-parser
    // 6. 1. Let result be the result of parsing part.
    // 6. 2. If result is failure, validation error, return failure.
    CHECK(upa::ipv4_parse(szEmpty, szEmpty, ipv4) != upa::validation_errc::ok);
}

TEST_CASE("IPv4 parser test with 127.0.0.1") {
    uint32_t ipv4;

    ipv4 = 0;
    CHECK(ipv4_parse("0x7f000001", ipv4) == upa::validation_errc::ok);
    CHECK(ipv4 == 0x7f000001);

    ipv4 = 0;
    CHECK(ipv4_parse("0x7f.0.0.1", ipv4) == upa::validation_errc::ok);
    CHECK(ipv4 == 0x7f000001);

    ipv4 = 0;
    CHECK(ipv4_parse("127.0.0.1", ipv4) == upa::validation_errc::ok);
    CHECK(ipv4 == 0x7f000001);

    ipv4 = 0;
    CHECK(ipv4_parse("127.0.1", ipv4) == upa::validation_errc::ok);
    CHECK(ipv4 == 0x7f000001);

    ipv4 = 0;
    CHECK(ipv4_parse("127.1", ipv4) == upa::validation_errc::ok);
    CHECK(ipv4 == 0x7f000001);
}

TEST_CASE("127.0.0.1 percent encoded") {
    upa::url url;

    // 127.0.0.1 percent encoded
    CHECK(upa::success(url.parse("http://12%37.0.0.1/", nullptr)));
    CHECK(url.hostname() == "127.0.0.1");

    // 0x7f.0.0.1 percent encoded
    CHECK(upa::success(url.parse("http://%30%78%37%66.0.0.1/", nullptr)));
    CHECK(url.hostname() == "127.0.0.1");
}
