// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url.h"
#include "doctest-main.h"
#include "test-utils.h"
#include <unordered_map>


std::string urls_to_str(const char* s1) {
    return s1;
}
std::string urls_to_str(const char* s1, const char* s2) {
    return std::string(s1) + " AGAINST " + s2;
}
std::string urls_to_str(const char* s1, const upa::url& u2) {
    return urls_to_str(s1, u2.to_string().c_str());
}

template <class ...Args>
void check_url_contructor(upa::validation_errc expected_res, Args&&... args)
{
    const auto stringify_args = [&] { return urls_to_str(args...); };
    try {
        upa::url url(args...);
        CHECK_MESSAGE(upa::validation_errc::ok == expected_res, "URL: ", stringify_args());
    }
    catch (upa::url_error& ex) {
        CHECK_MESSAGE(ex.result() == expected_res, "URL: ", stringify_args());
    }
    catch (...) {
        FAIL_CHECK("Unknown exception with URL: ", stringify_args());
    }
}

TEST_CASE("url constructor") {
    // Valid URL
    check_url_contructor(upa::validation_errc::ok, "http://example.org/p");

    // Invalid URLs (failure)

    // IDNA
    // https://url.spec.whatwg.org/#validation-error-domain-to-ascii
    check_url_contructor(upa::validation_errc::domain_to_ascii, "http://%C2%AD/p"); // MY: U+00AD - IDNA ignored code point
    check_url_contructor(upa::validation_errc::domain_to_ascii, "http://xn--a/p"); // MY

    // Host parsing
    // https://url.spec.whatwg.org/#domain-invalid-code-point
    check_url_contructor(upa::validation_errc::domain_invalid_code_point, "https://exa%23mple.org");
    check_url_contructor(upa::validation_errc::domain_invalid_code_point, "http://h[]/p"); // MY
    // https://url.spec.whatwg.org/#host-invalid-code-point
    check_url_contructor(upa::validation_errc::host_invalid_code_point, "foo://exa[mple.org");
    // https://url.spec.whatwg.org/#ipv4-too-many-parts
    check_url_contructor(upa::validation_errc::ipv4_too_many_parts, "https://1.2.3.4.5/");
    // https://url.spec.whatwg.org/#ipv4-non-numeric-part
    check_url_contructor(upa::validation_errc::ipv4_non_numeric_part, "https://test.42");
    // https://url.spec.whatwg.org/#ipv4-out-of-range-part
    check_url_contructor(upa::validation_errc::ipv4_out_of_range_part, "https://255.255.4000.1");
    check_url_contructor(upa::validation_errc::ipv4_out_of_range_part, "http://1.2.3.256/p"); // MY
    // https://url.spec.whatwg.org/#ipv6-unclosed
    check_url_contructor(upa::validation_errc::ipv6_unclosed, "https://[::1");
    // https://url.spec.whatwg.org/#ipv6-invalid-compression
    check_url_contructor(upa::validation_errc::ipv6_invalid_compression, "https://[:1]");
    check_url_contructor(upa::validation_errc::ipv6_invalid_compression, "https://[:]"); // MY
    // https://url.spec.whatwg.org/#ipv6-too-many-pieces
    check_url_contructor(upa::validation_errc::ipv6_too_many_pieces, "https://[1:2:3:4:5:6:7:8:9]");
    // https://url.spec.whatwg.org/#ipv6-multiple-compression
    check_url_contructor(upa::validation_errc::ipv6_multiple_compression, "https://[1::1::1]");
    // https://url.spec.whatwg.org/#ipv6-invalid-code-point
    check_url_contructor(upa::validation_errc::ipv6_invalid_code_point, "https://[1:2:3!:4]");
    check_url_contructor(upa::validation_errc::ipv6_invalid_code_point, "https://[1:2:3:]");
    check_url_contructor(upa::validation_errc::ipv6_invalid_code_point, "https://[-]"); // MY
    // https://url.spec.whatwg.org/#ipv6-too-few-pieces
    check_url_contructor(upa::validation_errc::ipv6_too_few_pieces, "https://[1:2:3]");
    check_url_contructor(upa::validation_errc::ipv6_too_few_pieces, "https://[]"); // MY
    check_url_contructor(upa::validation_errc::ipv6_too_few_pieces, "https://[F]"); // MY
    // https://url.spec.whatwg.org/#ipv4-in-ipv6-too-many-pieces
    check_url_contructor(upa::validation_errc::ipv4_in_ipv6_too_many_pieces, "https://[1:1:1:1:1:1:1:127.0.0.1]");
    // https://url.spec.whatwg.org/#ipv4-in-ipv6-invalid-code-point
    check_url_contructor(upa::validation_errc::ipv4_in_ipv6_invalid_code_point, "https://[ffff::.0.0.1]");
    check_url_contructor(upa::validation_errc::ipv4_in_ipv6_invalid_code_point, "https://[ffff::127.0.xyz.1]");
    check_url_contructor(upa::validation_errc::ipv4_in_ipv6_invalid_code_point, "https://[ffff::127.0xyz]");
    check_url_contructor(upa::validation_errc::ipv4_in_ipv6_invalid_code_point, "https://[ffff::127.00.0.1]");
    check_url_contructor(upa::validation_errc::ipv4_in_ipv6_invalid_code_point, "https://[ffff::127.0.0.1.2]");
    check_url_contructor(upa::validation_errc::ipv4_in_ipv6_invalid_code_point, "https://[.]"); // MY
    // https://url.spec.whatwg.org/#ipv4-in-ipv6-out-of-range-part
    check_url_contructor(upa::validation_errc::ipv4_in_ipv6_out_of_range_part, "https://[ffff::127.0.0.4000]");
    // https://url.spec.whatwg.org/#ipv4-in-ipv6-too-few-parts
    check_url_contructor(upa::validation_errc::ipv4_in_ipv6_too_few_parts, "https://[ffff::127.0.0]");

    // URL parsing
    // https://url.spec.whatwg.org/#missing-scheme-non-relative-url
    check_url_contructor(upa::validation_errc::missing_scheme_non_relative_url, "poomoji");
    check_url_contructor(upa::validation_errc::missing_scheme_non_relative_url, "poomoji", "mailto:user@example.org");
    // https://url.spec.whatwg.org/#host-missing
    check_url_contructor(upa::validation_errc::host_missing, "https://#fragment");
    check_url_contructor(upa::validation_errc::host_missing, "https://:443");
    check_url_contructor(upa::validation_errc::host_missing, "https://user:pass@");
    // https://url.spec.whatwg.org/#port-out-of-range
    check_url_contructor(upa::validation_errc::port_out_of_range, "https://example.org:70000");
    // https://url.spec.whatwg.org/#port-invalid
    check_url_contructor(upa::validation_errc::port_invalid, "https://example.org:7z");

    // Empty (invalid) base
    const upa::url base;
    check_url_contructor(upa::validation_errc::invalid_base, "http://h/", base);
}

// Copy/move construction/assignment

const char test_url[] = "http://h:123/p?a=b&c=d#frag";
const char test_rel_url[] = "//h:123/p?a=b&c=d#frag";
const char test_base_url[] = "http://example.org/p";
const pairs_list_t<std::string> test_url_params = { {"a","b"}, {"c","d"} };

void check_test_url(upa::url& url) {
    CHECK(url.href() == test_url);
    CHECK(url.origin() == "http://h:123");
    CHECK(url.protocol() == "http:");
    CHECK(url.host() == "h:123");
    CHECK(url.hostname() == "h");
    CHECK(url.port() == "123");
    CHECK(url.path() == "/p?a=b&c=d");
    CHECK(url.pathname() == "/p");
    CHECK(url.search() == "?a=b&c=d");
    CHECK(url.hash() == "#frag");
    CHECK(list_eq(url.search_params(), test_url_params));
}

TEST_CASE("url copy constructor") {
    upa::url url1(test_url);
    upa::url url2(url1);

    // CHECK(url2 == url1);
    check_test_url(url2);
}

TEST_CASE("url copy assignment") {
    upa::url url1(test_url);
    upa::url url2;

    url2 = url1;

    // CHECK(url2 == url1);
    check_test_url(url2);
}

TEST_CASE("url move constructor") {
    upa::url url0(test_url);
    upa::url url{ std::move(url0) };
    check_test_url(url);
}

TEST_CASE("url move assignment") {
    upa::url url;
    url = upa::url(test_url);
    check_test_url(url);
}

// url parsing constructor with base URL

TEST_CASE("url parsing constructor with base URL") {
    const upa::url base(test_base_url);
    {
        // pointer to base URL
        upa::url url(test_rel_url, &base);
        check_test_url(url);
    } {
        upa::url url(test_rel_url, base);
        check_test_url(url);
    }
}

TEST_CASE("url parsing constructor with base URL string") {
    upa::url url(test_rel_url, test_base_url);
    check_test_url(url);
}

// Parse URL

TEST_CASE("Two url::parse functions") {
    upa::url url;
    upa::url url_base;

    // first url::parse function
    CHECK(url_base.parse("http://example.org") == upa::validation_errc::ok);
    CHECK(url_base.href() == "http://example.org/");

    CHECK(url.parse("/htap", &url_base) == upa::validation_errc::ok);
    CHECK(url.href() == "http://example.org/htap");

    // second url::parse function
    CHECK(url.parse("/path", url_base) == upa::validation_errc::ok);
    CHECK(url.href() == "http://example.org/path");
}

TEST_CASE("url::parse must clear old URL data") {
    upa::url url;

    CHECK(upa::success(url.parse("about:blank")));
    CHECK_FALSE(url.empty());

    CHECK(upa::success(url.parse("http://host-1/")));
    CHECK(url.host("host-2"));

    CHECK(url.host() == "host-2");
    CHECK(url.href() == "http://host-2/");
}

// Can parse URL

TEST_CASE("url::can_parse") {
    SUBCASE("WPT: url-statics-canparse.any.js") {
        // Adapted from:
        // https://github.com/web-platform-tests/wpt/blob/master/url/url-statics-canparse.any.js
        // https://github.com/web-platform-tests/wpt/pull/39069
        CHECK_FALSE(upa::url::can_parse("undefined", nullptr));

        CHECK(upa::url::can_parse("aaa:b", nullptr));
        CHECK_FALSE(upa::url::can_parse("undefined", "aaa:b"));

        CHECK(upa::url::can_parse("aaa:/b", nullptr));
        CHECK(upa::url::can_parse("undefined", "aaa:/b"));

        CHECK_FALSE(upa::url::can_parse("https://test:test", nullptr));
        CHECK(upa::url::can_parse("a", "https://b/"));
    }

    SUBCASE("Additional tests") {
        const upa::url base("aaa:b");

        CHECK_FALSE(upa::url::can_parse("undefined", base));
        CHECK(upa::url::can_parse("aaa:/b", base));
    }
}

// Swap

TEST_CASE("swap(url&, url&)") {
    const std::string str_url_1 = "http://host-1:123/path-1?a=1&b=2#frag-1";
    const std::string str_url_2 = "http://host-2:321/path-2?c=3&d=4#frag-2";

    upa::url url_1(str_url_1);
    upa::url url_2(str_url_2);

    using std::swap;

    // Swap URLs with uninitialised search parameters
    swap(url_1, url_2);
    CHECK(url_1.href() == str_url_2);
    CHECK(url_2.href() == str_url_1);

    // Swap with one search parameter initialised
    auto& prm_1 = url_1.search_params();
    CHECK(prm_1.to_string() == "c=3&d=4");
    swap(url_1, url_2);
    CHECK(url_1.href() == str_url_1);
    CHECK(url_2.href() == str_url_2);

    // Initialise both search parameters
    CHECK(url_1.get_part_view(upa::url::QUERY) == url_1.search_params().to_string());
    CHECK(url_2.get_part_view(upa::url::QUERY) == url_2.search_params().to_string());

    // Swap with both search parameters initialised
    swap(url_1, url_2);
    CHECK(url_1.href() == str_url_2);
    CHECK(url_2.href() == str_url_1);
    CHECK(url_1.get_part_view(upa::url::QUERY) == url_1.search_params().to_string());
    CHECK(url_2.get_part_view(upa::url::QUERY) == url_2.search_params().to_string());

    // Are url and url_search_params still linked correctly?
    url_1.search_params().append("e", "10");
    url_2.search_params().append("f", "20");
    CHECK(url_1.get_part_view(upa::url::QUERY) == url_1.search_params().to_string());
    CHECK(url_2.get_part_view(upa::url::QUERY) == url_2.search_params().to_string());
}

// Valid or invalid URL

TEST_CASE("url::is_valid()") {
    // empty url invalid
    upa::url url;
    CHECK(url.empty());
    CHECK_FALSE(url.is_valid());

    // parse valid URL
    CHECK(url.parse("wss://host:88/path") == upa::validation_errc::ok);
    CHECK(url.href() == "wss://host:88/path");
    CHECK(url.is_valid());

    // href setter must not change original url on failure
    CHECK_FALSE(url.href("http://h:65616/p"));
    CHECK(url.href() == "wss://host:88/path");
    CHECK(url.is_valid());

    // url::parse must reset VALID_FLAG on failure
    CHECK(url.parse("http://h:8a/p") == upa::validation_errc::port_invalid);
    CHECK_FALSE(url.is_valid());

    // invalid URL must ignore setters (except href)
    std::string href{ url.href() };

    url.protocol("https");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.username("user");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.password("psw");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.host("host:1");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.hostname("host");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.port("12");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.pathname("path");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.search("a=b");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.search_params().append("c", "d");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.hash("hash");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    // href setter invoked with valid URL string as input
    // makes URL valid
    url.href("http://example.com/");
    CHECK(url.href() == "http://example.com/");
    CHECK_FALSE(url.empty());
    CHECK(url.is_valid());
}

TEST_CASE("Parse URL with invalid base") {
    SUBCASE("Empty base") {
        const upa::url base;
        CHECK_FALSE(base.is_valid());

        upa::url url;
        CHECK(url.parse("https://h/", base) == upa::validation_errc::invalid_base);
        CHECK_FALSE(url.is_valid());

        CHECK(url.parse("http://host/") == upa::validation_errc::ok);
        CHECK(url.is_valid());
        CHECK(url.parse("https://h/", base) == upa::validation_errc::invalid_base);
        CHECK_FALSE(url.is_valid());
    }
    SUBCASE("Invalid base") {
        upa::url base;
        CHECK_FALSE(upa::success(base.parse("http://h:65616/p")));
        CHECK_FALSE(base.is_valid());

        upa::url url;
        CHECK(url.parse("https://h/", base) == upa::validation_errc::invalid_base);
        CHECK_FALSE(url.is_valid());

        CHECK(url.parse("/path", base) == upa::validation_errc::invalid_base);
        CHECK_FALSE(url.is_valid());

        CHECK(url.parse("http://host/") == upa::validation_errc::ok);
        CHECK(url.is_valid());
        CHECK(url.parse("https://h/", base) == upa::validation_errc::invalid_base);
        CHECK_FALSE(url.is_valid());
    }
}

// Empty URL

TEST_CASE("Empty URL") {
    upa::url url;
    CHECK(url.empty());

    url.protocol("http");
    CHECK(url.empty());

    url.protocol("about");
    CHECK(url.empty());

    url.username("user");
    CHECK(url.empty());

    url.password("psw");
    CHECK(url.empty());

    url.host("hp:1");
    CHECK(url.empty());

    url.hostname("h");
    CHECK(url.empty());

    url.port("12");
    CHECK(url.empty());

    url.pathname("path");
    CHECK(url.empty());

    url.search("a=b");
    CHECK(url.empty());

    url.search_params().append("c", "d");
    CHECK(url.empty());

    url.hash("hash");
    CHECK(url.empty());

    // href setter invoked with valid URL string as input
    // makes URL not empty
    url.href("http://example.com/");
    CHECK_FALSE(url.empty());
    CHECK(url.href() == "http://example.com/");
}

// URL has an opaque path
// https://url.spec.whatwg.org/#url-opaque-path

TEST_CASE("url::has_opaque_path") {
    // Initially URL's path is empty list of URL path segments (non-opaque)
    // see: https://url.spec.whatwg.org/#concept-url-path
    upa::url url{};
    CHECK_FALSE(url.has_opaque_path());

    url.parse("about:blank");
    CHECK(url.has_opaque_path());

    url.parse("non-spec:/path");
    CHECK_FALSE(url.has_opaque_path());
}

// URL parts

TEST_CASE("url::is_empty and url::is_null") {
    upa::url url;

    CHECK(url.is_empty(upa::url::SCHEME));
    CHECK(url.is_null(upa::url::HOST));

    CHECK(upa::success(url.parse("http://example.org/")));
    CHECK_FALSE(url.is_empty(upa::url::SCHEME));
    CHECK_FALSE(url.is_null(upa::url::HOST));
}

// Origin tests

TEST_SUITE("Check origin") {
    TEST_CASE("http:") {
        upa::url url("http://host:123/path");
        CHECK(url.origin() == "http://host:123");
    }
    TEST_CASE("blob:") {
        upa::url url("blob:http://host:123/path");
        CHECK(url.origin() == "http://host:123");
    }
    TEST_CASE("blob: x 3") {
        upa::url url("blob:blob:blob:http://host:123/path");
        CHECK(url.origin() == "null");
    }
    TEST_CASE("file:") {
        upa::url url("file://host/path");
        CHECK(url.origin() == "null");
    }
    TEST_CASE("non-spec:") {
        upa::url url("non-spec://host:123/path");
        CHECK(url.origin() == "null");
    }
}

// URL serializing

static void check_serialize(std::string str_url, std::string str_hash) {
    upa::url u(str_url + str_hash);
    CHECK(u.serialize(false) == str_url + str_hash);
    CHECK(u.serialize(true) == str_url);
}

TEST_CASE("URL serializing") {
    check_serialize("http://h/", "");
    check_serialize("http://h/", "#");
    check_serialize("http://h/", "#f");
    check_serialize("http://h/?q", "");
    check_serialize("http://h/?q", "#");
    check_serialize("http://h/?q", "#f");
}

// URL equivalence

static bool are_equal(std::string atr_a, std::string atr_b, bool exclude_fragments) {
    upa::url a(atr_a);
    upa::url b(atr_b);
    return upa::equals(a, b, exclude_fragments);
}

TEST_CASE("URL equivalence") {
    CHECK(are_equal("http://h/#f", "http://h/#f", false));
    CHECK(are_equal("http://h/#f", "http://h/#f", true));

    CHECK_FALSE(are_equal("http://h/", "http://h/#", false));
    CHECK(are_equal("http://h/", "http://h/#", true));

    CHECK_FALSE(are_equal("http://h/", "http://h/#f", false));
    CHECK(are_equal("http://h/", "http://h/#f", true));

    CHECK_FALSE(are_equal("http://h/#", "http://h/#f", false));
    CHECK(are_equal("http://h/#", "http://h/#f", true));

    CHECK_FALSE(are_equal("http://h/#f1", "http://h/#f2", false));
    CHECK(are_equal("http://h/#f1", "http://h/#f2", true));

    CHECK_FALSE(are_equal("http://h1/", "http://h2/", false));
    CHECK_FALSE(are_equal("http://h1/", "http://h2/", true));
}

// UTF-8 in hostname

TEST_CASE("Valid UTF-8 in hostname") {
    static const char szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', char(0xC4), char(0x84), '/', '\0' }; // valid

    upa::url url;
    REQUIRE(upa::success(url.parse(szUrl)));
    CHECK(url.hostname() == "xn--2da");
}
TEST_CASE("Valid percent encoded utf-8 in hostname") {
    static const char szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', '%', 'C', '4', '%', '8', '4', '/', '\0' }; // valid

    upa::url url;
    REQUIRE(upa::success(url.parse(szUrl)));
    CHECK(url.hostname() == "xn--2da");
}
TEST_CASE("Invalid utf-8 in hostname") {
    static const char szUrl1[] = { 'h', 't', 't', 'p', ':', '/', '/', '%', 'C', '4', char(0x84), '/', '\0' }; // invalid
    static const char szUrl2[] = { 'h', 't', 't', 'p', ':', '/', '/', char(0xC4), '%', '8', '4', '/', '\0' }; // invalid

    check_url_contructor(upa::validation_errc::domain_to_ascii, szUrl1);
    check_url_contructor(upa::validation_errc::domain_to_ascii, szUrl2);
}

// UTF-16 in hostname

TEST_CASE("Valid UTF-16 in hostname") {
    static const char16_t szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', char16_t(0xD800), char16_t(0xDC00), '/', '\0' };

    upa::url url;
    REQUIRE(upa::success(url.parse(szUrl)));
    CHECK(url.hostname() == "xn--2n7c");
}
TEST_CASE("Invalid UTF-16 in hostname") {
    static const char16_t szUrl1[] = { 'h', 't', 't', 'p', ':', '/', '/', char16_t(0xD800), '/', '\0' };
    static const char16_t szUrl2[] = { 'h', 't', 't', 'p', ':', '/', '/', char16_t(0xDC00), '/', '\0' };

    CHECK_THROWS_AS(upa::url{ szUrl1 }, upa::url_error);
    CHECK_THROWS_AS(upa::url{ szUrl2 }, upa::url_error);
}

// UTF-32 in hostname

TEST_CASE("Valid UTF-32 in hostname") {
    static const char32_t szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0x10000u), '/', '\0' };

    upa::url url;
    REQUIRE(upa::success(url.parse(szUrl)));
    CHECK(url.hostname() == "xn--2n7c");
}
TEST_CASE("Invalid UTF-32 in hostname") {
    static const char32_t szUrl1[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0xD800), '/', '\0' };
    static const char32_t szUrl2[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0xDFFF), '/', '\0' };
    static const char32_t szUrl3[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0x110000u), '/', '\0' };

    CHECK_THROWS_AS(upa::url{ szUrl1 }, upa::url_error);
    CHECK_THROWS_AS(upa::url{ szUrl2 }, upa::url_error);
    CHECK_THROWS_AS(upa::url{ szUrl3 }, upa::url_error);
}

// URL utilities

TEST_CASE("url_from_file_path") {
    SUBCASE("POSIX path") {
        CHECK(upa::url_from_file_path("/").href() == "file:///");
        CHECK(upa::url_from_file_path("/path").href() == "file:///path");
        CHECK(upa::url_from_file_path("/path %#?").href() == "file:///path%20%25%23%3F");
        CHECK(upa::url_from_file_path("/c:\\end").href() == "file:///c%3A%5Cend");
        CHECK(upa::url_from_file_path("/c|\\end").href() == "file:///c%7C%5Cend");
        CHECK(upa::url_from_file_path("/c:/last").href() == "file:///c%3A/last");
        CHECK(upa::url_from_file_path("/c|/last").href() == "file:///c%7C/last");
        CHECK(upa::url_from_file_path("/\\", upa::file_path_format::posix).href() == "file:///%5C");
        // empty path
        CHECK_THROWS_AS(upa::url_from_file_path(""), upa::url_error);
        // non absolute path
        CHECK_THROWS_AS(upa::url_from_file_path("path"), upa::url_error);
        CHECK_THROWS_AS(upa::url_from_file_path("\\\\h\\p", upa::file_path_format::posix), upa::url_error);
        // null character
        CHECK_THROWS_AS(upa::url_from_file_path(std::string{ "/p\0", 3 }, upa::file_path_format::posix), upa::url_error);
    }
    SUBCASE("Windows path") {
        // https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file
        CHECK(upa::url_from_file_path("C:\\").href() == "file:///C:/");
        CHECK(upa::url_from_file_path("C:\\path").href() == "file:///C:/path");
        CHECK(upa::url_from_file_path("C|\\path").href() == "file:///C:/path");
        CHECK(upa::url_from_file_path("C:/path").href() == "file:///C:/path");
        CHECK(upa::url_from_file_path("C:\\path %#").href() == "file:///C:/path%20%25%23");
        CHECK(upa::url_from_file_path("\\\\h\\path").href() == "file://h/path");
        CHECK(upa::url_from_file_path("\\\\h\\a/b").href() == "file://h/a/b");
        CHECK(upa::url_from_file_path("\\\\a/b\\path").href() == "file://a/b/path");
        CHECK(upa::url_from_file_path("//h/path", upa::file_path_format::windows).href() == "file://h/path");
        // https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats
        // https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
        CHECK(upa::url_from_file_path("\\\\?\\D:\\very_long_path").href() == "file:///D:/very_long_path");
        CHECK(upa::url_from_file_path("\\\\?\\UNC\\h\\very_long_path").href() == "file://h/very_long_path");
        CHECK(upa::url_from_file_path("\\\\?/unc/h/very_long_path").href() == "file://h/very_long_path");
        CHECK(upa::url_from_file_path("\\\\.\\D:\\just_path").href() == "file:///D:/just_path");
        CHECK(upa::url_from_file_path("\\\\.\\UNC\\h\\just_path").href() == "file://h/just_path");
        CHECK(upa::url_from_file_path("\\\\./unc/h/just_path").href() == "file://h/just_path");
        CHECK(upa::url_from_file_path("//?/unc/h/very_long_path", upa::file_path_format::windows).href() == "file://h/very_long_path");
        CHECK(upa::url_from_file_path("//./unc/h/just_path", upa::file_path_format::windows).href() == "file://h/just_path");
        // non absolute path
        CHECK_THROWS_AS(upa::url_from_file_path("\\"), upa::url_error);
        CHECK_THROWS_AS(upa::url_from_file_path("C:path"), upa::url_error);
        CHECK_THROWS_AS(upa::url_from_file_path("/", upa::file_path_format::windows), upa::url_error);
        // invalid UNC
        CHECK_THROWS_AS(upa::url_from_file_path("\\\\"), upa::url_error);
        CHECK_THROWS_AS(upa::url_from_file_path("\\\\h"), upa::url_error);
        CHECK_THROWS_AS(upa::url_from_file_path("\\\\h\\"), upa::url_error);
        CHECK_THROWS_AS(upa::url_from_file_path("\\\\h\\\\"), upa::url_error);
        CHECK_THROWS_AS(upa::url_from_file_path(std::string{ '\\', '\\', 'h', '\\', 'a', '\0', 'b' }), upa::url_error);
        CHECK_THROWS_AS(upa::url_from_file_path("\\\\C:\\path"), upa::url_error);
        CHECK_THROWS_AS(upa::url_from_file_path("\\\\C|\\path"), upa::url_error);
        // invalid hostname
        CHECK_THROWS_AS(upa::url_from_file_path("\\\\a b\\path"), upa::url_error);
        // unsupported pathes
        CHECK_THROWS_AS(upa::url_from_file_path("\\\\?\\Volume{b75e2c83-0000-0000-0000-602f00000000}\\Test\\Foo.txt"), upa::url_error);
        CHECK_THROWS_AS(upa::url_from_file_path("\\\\.\\Volume{b75e2c83-0000-0000-0000-602f00000000}\\Test\\Foo.txt"), upa::url_error);
        // null character
        CHECK_THROWS_AS(upa::url_from_file_path(std::string{ "/C:/p\0", 6 }, upa::file_path_format::posix), upa::url_error);
    }
}

TEST_CASE("path_from_file_url") {
    const auto path_from_file_url = [](const char* str_url, upa::file_path_format format) -> std::string {
        return upa::path_from_file_url(upa::url(str_url), format);
    };
    const auto path_from_file_url_1 = [](const char* str_url) -> std::string {
        return upa::path_from_file_url(upa::url(str_url));
    };

    SUBCASE("POSIX path") {
        CHECK(path_from_file_url("file:///", upa::file_path_format::posix) == "/");
        CHECK(path_from_file_url("file:///path", upa::file_path_format::posix) == "/path");
        // POSIX path cannot have host
        CHECK_THROWS_AS(path_from_file_url("file://host/path", upa::file_path_format::posix), upa::url_error);
        // null character
        CHECK_THROWS_AS(path_from_file_url("file:///p%00", upa::file_path_format::posix), upa::url_error);
    }
    SUBCASE("Windows path") {
        CHECK(path_from_file_url("file:///C:", upa::file_path_format::windows) == "C:\\");
        CHECK(path_from_file_url("file:///C%3A", upa::file_path_format::windows) == "C:\\");
        CHECK(path_from_file_url("file:///C:?", upa::file_path_format::windows) == "C:\\");
        CHECK(path_from_file_url("file:///C:#", upa::file_path_format::windows) == "C:\\");
        CHECK(path_from_file_url("file:///C:/", upa::file_path_format::windows) == "C:\\");
        CHECK(path_from_file_url("file:///C:/path", upa::file_path_format::windows) == "C:\\path");
        CHECK(path_from_file_url("file:///C%3A%5Cpath", upa::file_path_format::windows) == "C:\\path");
        // Not a Windows path
        CHECK_THROWS_AS(path_from_file_url("file:///", upa::file_path_format::windows), upa::url_error);
        CHECK_THROWS_AS(path_from_file_url("file:///p", upa::file_path_format::windows), upa::url_error);
        CHECK_THROWS_AS(path_from_file_url("file:///h/p", upa::file_path_format::windows), upa::url_error);
        CHECK_THROWS_AS(path_from_file_url("file://////h/p", upa::file_path_format::windows), upa::url_error);
        // UNC
        CHECK(path_from_file_url("file://host/path", upa::file_path_format::windows) == "\\\\host\\path");
        CHECK(path_from_file_url("file:////host/path", upa::file_path_format::windows) == "\\\\host\\path");
        CHECK(path_from_file_url("file://///host/path", upa::file_path_format::windows) == "\\\\host\\path");
        // Invalid UNC
        CHECK_THROWS_AS(path_from_file_url("file://host", upa::file_path_format::windows), upa::url_error);
        CHECK_THROWS_AS(path_from_file_url("file://host/", upa::file_path_format::windows), upa::url_error);
        CHECK_THROWS_AS(path_from_file_url("file:////host/", upa::file_path_format::windows), upa::url_error);
        CHECK_THROWS_AS(path_from_file_url("file://///host/", upa::file_path_format::windows), upa::url_error);
        // Unsupported "." hostname
        CHECK_THROWS_AS(path_from_file_url("file://./name", upa::file_path_format::windows), upa::url_error);
        // null character
        CHECK_THROWS_AS(path_from_file_url("file:///C:/p%00", upa::file_path_format::posix), upa::url_error);
    }
    SUBCASE("Native path") {
#ifdef _WIN32
        CHECK(path_from_file_url_1("file:///C:") == "C:\\");
        CHECK(path_from_file_url("file:///C:", upa::file_path_format::native) == "C:\\");
        CHECK(path_from_file_url("file:///C:", upa::file_path_format::detect) == "C:\\");
#else
        CHECK(path_from_file_url_1("file:///") == "/");
        CHECK(path_from_file_url("file:///", upa::file_path_format::native) == "/");
        CHECK(path_from_file_url("file:///", upa::file_path_format::detect) == "/");
#endif
    }
    SUBCASE("Not a file URL") {
        CHECK_THROWS_AS(path_from_file_url("non-spec:///", upa::file_path_format::posix), upa::url_error);
        CHECK_THROWS_AS(path_from_file_url("non-spec:///c:/", upa::file_path_format::windows), upa::url_error);
        CHECK_THROWS_AS(path_from_file_url("http://host/path", upa::file_path_format::windows), upa::url_error);
    }
}

// Test std::hash specialization and operator==

TEST_CASE("std::hash<upa::url> and operator==") {
    std::unordered_map<upa::url, int> map;

    map.emplace(upa::url{ "about:blank" }, 1);
    map.emplace(upa::url{ "file:///path" }, 2);
    map.emplace(upa::url{ "https://example.org/" }, 3);

    CHECK(map.at(upa::url{ "about:blank" }) == 1);
    CHECK(map.at(upa::url{ "file:///path" }) == 2);
    CHECK(map.at(upa::url{ "https://example.org/" }) == 3);

    CHECK(upa::url{ "about:blank" } == upa::url{ "about:blank" });
    CHECK_FALSE(upa::url{ "about:blank" } == upa::url{ "https://example.org/" });
}
