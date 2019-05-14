#include "url.h"
#include "doctest-main.h"
#include <algorithm>
#include <cassert>
#include <initializer_list>


static bool ipv6_parse(const char* szInput, uint16_t(&address)[8]) {
    using str = std::char_traits<char>;
    return whatwg::ipv6_parse(szInput, szInput + str::length(szInput), address);
};

static std::string ipv6_serialize(uint16_t(&address)[8]) {
    std::string strout;
    whatwg::ipv6_serialize(address, strout);
    return strout;
}

static bool is_equal(uint16_t(&a)[8], std::initializer_list<uint16_t> b) {
    assert(b.size() == 8);
    return std::equal(std::begin(a), std::end(a), std::begin(b));
}

TEST_CASE("IPv6 parser test with empty input") {
    uint16_t ipv6addr[8];

    // https://url.spec.whatwg.org/#concept-ipv6-parser
    // 8. Otherwise, if compress is null and pieceIndex is not 8, validation error, return failure.
    CHECK_FALSE(ipv6_parse("", ipv6addr));
}

TEST_CASE("IPv6 parser test with valid addresses") {
    uint16_t ipv6addr[8];

    CHECK(ipv6_parse("1:2:3:4:5:6:7:8", ipv6addr));
    CHECK(is_equal(ipv6addr, { 1, 2, 3, 4, 5, 6, 7, 8 }));
    CHECK(ipv6_serialize(ipv6addr) == "1:2:3:4:5:6:7:8");

    // rust-url bug (fixed)
    CHECK(ipv6_parse("1:2:3:4::6:7:8", ipv6addr));
    CHECK(is_equal(ipv6addr, { 1, 2, 3, 4, 0, 6, 7, 8 }));
    CHECK(ipv6_serialize(ipv6addr) == "1:2:3:4:0:6:7:8");

    CHECK(ipv6_parse("1:2::7:8", ipv6addr));
    CHECK(is_equal(ipv6addr, { 1, 2, 0, 0, 0, 0, 7, 8 }));
    CHECK(ipv6_serialize(ipv6addr) == "1:2::7:8");

    CHECK(ipv6_parse("1:2:3::", ipv6addr));
    CHECK(is_equal(ipv6addr, { 1, 2, 3, 0, 0, 0, 0, 0 }));
    CHECK(ipv6_serialize(ipv6addr) == "1:2:3::");

    CHECK(ipv6_parse("::6:7:8", ipv6addr));
    CHECK(is_equal(ipv6addr, { 0, 0, 0, 0, 0, 6, 7, 8 }));
    CHECK(ipv6_serialize(ipv6addr) == "::6:7:8");

    CHECK(ipv6_parse("0::0", ipv6addr));
    CHECK(is_equal(ipv6addr, { 0, 0, 0, 0, 0, 0, 0, 0 }));
    CHECK(ipv6_serialize(ipv6addr) == "::");

    CHECK(ipv6_parse("::", ipv6addr));
    CHECK(is_equal(ipv6addr, { 0, 0, 0, 0, 0, 0, 0, 0 }));
    CHECK(ipv6_serialize(ipv6addr) == "::");

    CHECK(ipv6_parse("0:f:0:0:f:f:0:0", ipv6addr));
    CHECK(is_equal(ipv6addr, { 0, 0xf, 0, 0, 0xf, 0xf, 0, 0 }));
    CHECK(ipv6_serialize(ipv6addr) == "0:f::f:f:0:0");
}

TEST_CASE("IPv4 in IPv6 test") {
    uint16_t ipv6addr[8];

    CHECK(ipv6_parse("::1.2.3.4", ipv6addr));
    CHECK(is_equal(ipv6addr, { 0, 0, 0, 0, 0, 0, 0x0102, 0x0304 }));
    CHECK(ipv6_serialize(ipv6addr) == "::102:304");

    // bounds checking
    CHECK_FALSE(ipv6_parse("::1.2.3.4.5", ipv6addr));
    CHECK_FALSE(ipv6_parse("1:2:3:4:5:6:1.2.3.4.5", ipv6addr));

    // https://github.com/whatwg/url/issues/195
    CHECK_FALSE(ipv6_parse("::1.2.3.4x", ipv6addr));
    CHECK_FALSE(ipv6_parse("::1.2.3.", ipv6addr));
    CHECK_FALSE(ipv6_parse("::1.2.", ipv6addr));
    CHECK_FALSE(ipv6_parse("::1.", ipv6addr));
}
