#include "url.h"
#include "doctest-main.h"


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
