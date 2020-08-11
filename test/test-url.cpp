#include "url.h"
#include "doctest-main.h"
#include "test-utils.h"


const char* urls_to_str(const char* s1) {
    return s1;
}
std::string urls_to_str(const char* s1, const char* s2) {
    return std::string(s1) + " AGAINST " + s2;
}

template <class ...Args>
void check_url_contructor(whatwg::url_result expected_res, Args&&... args)
{
    try {
        whatwg::url url(args...);
        CHECK_MESSAGE(whatwg::url_result::Ok == expected_res, "URL: " << urls_to_str(args...));
    }
    catch (whatwg::url_error& ex) {
        CHECK_MESSAGE(ex.result() == expected_res, "URL: " << urls_to_str(args...));
    }
    catch (...) {
        FAIL_CHECK("Unknown exception with URL:" << urls_to_str(args...));
    }
}

TEST_CASE("url constructor") {
    // valid URL
    check_url_contructor(whatwg::url_result::Ok, "http://example.org/p");
    // invalid URLs
    check_url_contructor(whatwg::url_result::EmptyHost, "http://xn--/p");
    check_url_contructor(whatwg::url_result::IdnaError, "http://xn--a/p");
    check_url_contructor(whatwg::url_result::InvalidPort, "http://h:a/p");
    check_url_contructor(whatwg::url_result::InvalidIpv4Address, "http://1.2.3.256/p");
    check_url_contructor(whatwg::url_result::InvalidIpv6Address, "http://[1::2::3]/p");
    check_url_contructor(whatwg::url_result::InvalidDomainCharacter, "http://h[]/p");
    check_url_contructor(whatwg::url_result::RelativeUrlWithoutBase, "relative");
    check_url_contructor(whatwg::url_result::RelativeUrlWithCannotBeABase, "relative", "about:blank");
}

// Copy/move construction/assignment

const char* test_url = "http://h:123/p?a=b&c=d#frag";
const char* test_rel_url = "//h:123/p?a=b&c=d#frag";
const pairs_list_t<std::string> test_url_params = { {"a","b"}, {"c","d"} };

void check_test_url(whatwg::url& url) {
    CHECK(url.href() == test_url);
    CHECK(url.protocol() == "http:");
    CHECK(url.host() == "h:123");
    CHECK(url.hostname() == "h");
    CHECK(url.port() == "123");
    CHECK(url.path() == "/p?a=b&c=d");
    CHECK(url.pathname() == "/p");
    CHECK(url.search() == "?a=b&c=d");
    CHECK(url.hash() == "#frag");
    CHECK(list_eq(url.searchParams(), test_url_params));
}

TEST_CASE("url copy constructor") {
    whatwg::url url1(test_url);
    whatwg::url url2(url1);

    // CHECK(url2 == url1);
    check_test_url(url2);
}

TEST_CASE("url copy assignment") {
    whatwg::url url1(test_url);
    whatwg::url url2;

    url2 = url1;

    // CHECK(url2 == url1);
    check_test_url(url2);
}

TEST_CASE("url move constructor") {
    whatwg::url url0(test_url);
    whatwg::url url{ std::move(url0) };
    check_test_url(url);
}

TEST_CASE("url move assignment") {
    whatwg::url url;
    url = whatwg::url(test_url);
    check_test_url(url);
}

// url parsing constructor with base URL

TEST_CASE("url parsing constructor with base URL") {
    const whatwg::url base("http://example.org/p");
    {
        // pointer to base URL
        whatwg::url url(test_rel_url, &base);
        check_test_url(url);
    } {
        whatwg::url url(test_rel_url, base);
        check_test_url(url);
    }
}

// UTF-8 in hostname

TEST_CASE("Valid UTF-8 in hostname") {
    static const char szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', char(0xC4), char(0x84), '/', '\0' }; // valid

    whatwg::url url;
    REQUIRE(whatwg::success(url.parse(szUrl, nullptr)));
    CHECK(url.hostname() == "xn--2da");
}
TEST_CASE("Valid percent encoded utf-8 in hostname") {
    static const char szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', '%', 'C', '4', '%', '8', '4', '/', '\0' }; // valid

    whatwg::url url;
    REQUIRE(whatwg::success(url.parse(szUrl, nullptr)));
    CHECK(url.hostname() == "xn--2da");
}
TEST_CASE("Invalid utf-8 in hostname") {
    static const char szUrl1[] = { 'h', 't', 't', 'p', ':', '/', '/', '%', 'C', '4', char(0x84), '/', '\0' }; // invalid
    static const char szUrl2[] = { 'h', 't', 't', 'p', ':', '/', '/', char(0xC4), '%', '8', '4', '/', '\0' }; // invalid

    check_url_contructor(whatwg::url_result::IdnaError, szUrl1);
    check_url_contructor(whatwg::url_result::IdnaError, szUrl2);
}

// UTF-16 in hostname

TEST_CASE("Valid UTF-16 in hostname") {
    static const char16_t szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', char16_t(0xD800), char16_t(0xDC00), '/', '\0' };

    whatwg::url url;
    REQUIRE(whatwg::success(url.parse(szUrl, nullptr)));
    CHECK(url.hostname() == "xn--2n7c");
}
TEST_CASE("Invalid UTF-16 in hostname") {
    static const char16_t szUrl1[] = { 'h', 't', 't', 'p', ':', '/', '/', char16_t(0xD800), '/', '\0' };
    static const char16_t szUrl2[] = { 'h', 't', 't', 'p', ':', '/', '/', char16_t(0xDC00), '/', '\0' };

    CHECK_THROWS_AS(whatwg::url{ szUrl1 }, whatwg::url_error);
    CHECK_THROWS_AS(whatwg::url{ szUrl2 }, whatwg::url_error);
}

// UTF-32 in hostname

TEST_CASE("Valid UTF-32 in hostname") {
    static const char32_t szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0x10000u), '/', '\0' };

    whatwg::url url;
    REQUIRE(whatwg::success(url.parse(szUrl, nullptr)));
    CHECK(url.hostname() == "xn--2n7c");
}
TEST_CASE("Invalid UTF-32 in hostname") {
    static const char32_t szUrl1[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0xD800), '/', '\0' };
    static const char32_t szUrl2[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0xDFFF), '/', '\0' };
    static const char32_t szUrl3[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0x110000u), '/', '\0' };

    CHECK_THROWS_AS(whatwg::url{ szUrl1 }, whatwg::url_error);
    CHECK_THROWS_AS(whatwg::url{ szUrl2 }, whatwg::url_error);
    CHECK_THROWS_AS(whatwg::url{ szUrl3 }, whatwg::url_error);
}
