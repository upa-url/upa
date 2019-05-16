#include "url_search_params.h"
#include "doctest-main.h"

// Tests based on "urlsearchparams-*.any.js" files from
// https://github.com/web-platform-tests/wpt/tree/master/url


static bool param_eq(const std::string* pval, const char* value) {
    return pval != nullptr && *pval == value;
}

//
// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-append.any.js
//
TEST_CASE("urlsearchparams-append.any.js") {
    SUBCASE("Append same name") {
        whatwg::url_search_params params;

        params.append("a", "b");
        CHECK(params.to_string() == "a=b");

        params.append("a", "b");
        CHECK(params.to_string() == "a=b&a=b");

        params.append("a", "c");
        CHECK(params.to_string() == "a=b&a=b&a=c");
    }

    SUBCASE("Append empty strings") {
        whatwg::url_search_params params;

        params.append("", "");
        CHECK(params.to_string() == "=");
        params.append("", "");
        CHECK(params.to_string() == "=&=");
    }

#if 0
    // TODO: fix whatwg::url_search_params
    // https://developer.mozilla.org/en-US/docs/Web/API/USVString
    // https://developer.mozilla.org/en-US/docs/Web/API/DOMString
    // Passing null to a method or parameter accepting a DOMString typically stringifies to "null".
    SUBCASE("Append null") {
        whatwg::url_search_params params;

        params.append(nullptr, nullptr);
        CHECK(params.to_string() == "null=null");

        params.append(nullptr, nullptr);
        CHECK(params.to_string() == "null=null&null=null");
    }
#endif

    SUBCASE("Append multiple") {
        whatwg::url_search_params params;

        params.append("first", "1"); // TODO: 1
        params.append("second", "2"); // TODO: 2
        params.append("third", "");
        params.append("first", "10"); // TODO: 10

        CHECK_MESSAGE(params.has("first"), "Search params object has name \"first\"");
        CHECK_MESSAGE(param_eq(params.get("first"), "1"), "Search params object has name \"first\" with value \"1\"");
        CHECK_MESSAGE(param_eq(params.get("second"), "2"), "Search params object has name \"second\" with value \"2\"");
        CHECK_MESSAGE(param_eq(params.get("third"), ""), "Search params object has name \"third\" with value \"\"");
        params.append("first", "10"); // TODO: 10
        CHECK_MESSAGE(param_eq(params.get("first"), "1"), "Search params object has name \"first\" with value \"1\"");
    }
}
