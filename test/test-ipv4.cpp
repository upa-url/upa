#include "url.h"
#include "doctest-main.h"

using str = std::char_traits<char>;

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
