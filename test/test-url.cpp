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
