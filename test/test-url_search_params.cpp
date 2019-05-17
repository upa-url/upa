#include "url_search_params.h"
#include "doctest-main.h"

// Tests based on "urlsearchparams-*.any.js" files from
// https://github.com/web-platform-tests/wpt/tree/master/url

template <class T>
static bool param_eq(const std::string* pval, const T& value) {
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

//
// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-constructor.any.js
//
TEST_CASE("urlsearchparams-constructor.any.js") {
    SUBCASE("Basic URLSearchParams construction") {
        {
            whatwg::url_search_params params;
            CHECK_EQ(params.to_string(), "");
        }
        {
            whatwg::url_search_params params("");
            CHECK_EQ(params.to_string(), "");
        }
        {
            whatwg::url_search_params params("a=b");
            CHECK_EQ(params.to_string(), "a=b");

            // copy constructor
            whatwg::url_search_params paramsC(params);
            CHECK_EQ(paramsC.to_string(), "a=b");

            // move constructor
            whatwg::url_search_params paramsM(std::move(params));
            CHECK_EQ(paramsM.to_string(), "a=b");
        }
    }

    // The first SUBCASE contains the same Check
    SUBCASE("URLSearchParams constructor, no arguments") {
        whatwg::url_search_params params;
        CHECK_EQ(params.to_string(), "");
    }

    SUBCASE("URLSearchParams constructor, remove leading \"?\"") {
        whatwg::url_search_params params("?a=b");
        CHECK_EQ(params.to_string(), "a=b");
    }

    // Skip JavaScript specific tests:
    // 1) new URLSearchParams(DOMException)
    // 2) assert_equals(params.__proto__, URLSearchParams.prototype, ...)
    // 3) new URLSearchParams({}); TODO: maybe empty initializer_list?

    SUBCASE("URLSearchParams constructor, string.") {
        {
            whatwg::url_search_params params("a=b");
            CHECK_MESSAGE(params.has("a"), "Search params object has name \"a\"");
            CHECK_FALSE_MESSAGE(params.has("b"), "Search params object has not got name \"b\"");
        }
        {
            whatwg::url_search_params params("a=b&c");
            CHECK_MESSAGE(params.has("a"), "Search params object has name \"a\"");
            CHECK_MESSAGE(params.has("c"), "Search params object has name \"c\"");
        }
        {
            whatwg::url_search_params params("&a&&& &&&&&a+b=& c&m%c3%b8%c3%b8");
            CHECK_MESSAGE(params.has("a"), "Search params object has name \"a\"");
            CHECK_MESSAGE(params.has("a b"), "Search params object has name \"a b\"");
            CHECK_MESSAGE(params.has(" "), "Search params object has name \" \"");
            CHECK_FALSE_MESSAGE(params.has("c"), "Search params object did not have the name \"c\"");
            CHECK_MESSAGE(params.has(" c"), "Search params object has name \" c\"");
            CHECK_MESSAGE(params.has(u8"m\u00F8\u00F8"), u8"Search params object has name \"m\u00F8\u00F8\"");
        }
    }

    SUBCASE("URLSearchParams constructor, object.") {
        whatwg::url_search_params seed("a=b&c=d");
        whatwg::url_search_params params(seed);
        CHECK(param_eq(params.get("a"), "b"));
        CHECK(param_eq(params.get("c"), "d"));
        CHECK_FALSE(params.has("d"));
        // The name-value pairs are copied when created; later updates
        // should not be observable.
        seed.append("e", "f");
        CHECK_FALSE(params.has("e"));
        params.append("g", "h");
        CHECK_FALSE(seed.has("g"));
    }

    SUBCASE("Parse +") {
        {
            whatwg::url_search_params params("a=b+c");
            CHECK(param_eq(params.get("a"), "b c"));
        }
        {
            whatwg::url_search_params params("a+b=c");
            CHECK(param_eq(params.get("a b"), "c"));
        }
    }

    SUBCASE("Parse encoded +") {
        const std::string testValue = "+15555555555";

        whatwg::url_search_params params;
        params.set("query", testValue);
        whatwg::url_search_params newParams(params.to_string());

        CHECK_EQ(params.to_string(), "query=%2B15555555555");
        CHECK(param_eq(params.get("query"), testValue));
        CHECK(param_eq(newParams.get("query"), testValue));
    }

    SUBCASE("Parse space") {
        {
            whatwg::url_search_params params("a=b c");
            CHECK(param_eq(params.get("a"), "b c"));
        }
        {
            whatwg::url_search_params params("a b=c");
            CHECK(param_eq(params.get("a b"), "c"));
        }
    }

    SUBCASE("Parse %20") {
        {
            whatwg::url_search_params params("a=b%20c");
            CHECK(param_eq(params.get("a"), "b c"));
        }
        {
            whatwg::url_search_params params("a%20b=c");
            CHECK(param_eq(params.get("a b"), "c"));
        }
    }

    // TODO:
    // C++14 has string literal: "..."s
    // https://en.cppreference.com/w/cpp/string/basic_string/operator%22%22s
    // C++17 has string_view literal: "..."sv
    // https://en.cppreference.com/w/cpp/string/basic_string_view/operator%22%22sv
    SUBCASE("Parse \\0") {
        {
            whatwg::url_search_params params(std::string("a=b\0c", 5));
            CHECK(param_eq(params.get("a"), std::string("b\0c", 3)));
        }
        {
            whatwg::url_search_params params(std::string("a\0b=c", 5));
            CHECK(param_eq(params.get(std::string("a\0b", 3)), "c"));
        }
    }

    SUBCASE("Parse %00") {
        {
            whatwg::url_search_params params("a=b%00c");
            CHECK(param_eq(params.get("a"), std::string("b\0c", 3)));
        }
        {
            whatwg::url_search_params params("a%00b=c");
            CHECK(param_eq(params.get(std::string("a\0b", 3)), "c"));
        }
    }

    // Unicode Character 'COMPOSITION SYMBOL' (U+2384)
    SUBCASE(u8"Parse \u2384") {
        {
            whatwg::url_search_params params(u8"a=b\u2384");
            CHECK(param_eq(params.get("a"), u8"b\u2384"));
        }
        {
            whatwg::url_search_params params(u8"a\u2384b=c");
            CHECK(param_eq(params.get(u8"a\u2384b"), "c"));
        }
    }

    // Unicode Character 'COMPOSITION SYMBOL' (U+2384)
    SUBCASE("Parse %e2%8e%84") {
        {
            whatwg::url_search_params params("a=b%e2%8e%84");
            CHECK(param_eq(params.get("a"), u8"b\u2384"));
        }
        {
            whatwg::url_search_params params("a%e2%8e%84b=c");
            CHECK(param_eq(params.get(u8"a\u2384b"), "c"));
        }
    }

    // Unicode Character 'PILE OF POO' (U+1F4A9)
    SUBCASE(u8"Parse \U0001F4A9") {
        {
            whatwg::url_search_params params(u8"a=b\U0001F4A9");
            CHECK(param_eq(params.get("a"), u8"b\U0001F4A9"));
        }
        {
            whatwg::url_search_params params(u8"a\U0001F4A9b=c");
            CHECK(param_eq(params.get(u8"a\U0001F4A9b"), "c"));
        }
    }

    // Unicode Character 'PILE OF POO' (U+1F4A9)
    SUBCASE("Parse %f0%9f%92%a9") {
        {
            whatwg::url_search_params params("a=b%f0%9f%92%a9c");
            CHECK(param_eq(params.get("a"), u8"b\U0001F4A9c"));
        }
        {
            whatwg::url_search_params params("a%f0%9f%92%a9b=c");
            CHECK(param_eq(params.get(u8"a\U0001F4A9b"), "c"));
        }
    }

#if 0
    // TODO
    SUBCASE("Constructor with sequence of sequences of strings") {
        {
            // TODO
            //whatwg::url_search_params params([]);
            whatwg::url_search_params params([["a", "b"], ["c", "d"]]);
            CHECK(param_eq(params.get("a"), "b"));
            CHECK(param_eq(params.get("c"), "d"));
            //assert_throws(new TypeError(), function(){ new URLSearchParams([[1]]); });
            //assert_throws(new TypeError(), function(){ new URLSearchParams([[1,2,3]]); });
        }
    }
#endif
    // TODO: other tests
}

//
// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-delete.any.js
//
TEST_CASE("urlsearchparams-delete.any.js") {
    SUBCASE("Delete basics") {
        {
            whatwg::url_search_params params("a=b&c=d");
            params.del("a");
            CHECK_EQ(params.to_string(), "c=d");
        }
        {
            whatwg::url_search_params params("a=a&b=b&a=a&c=c");
            params.del("a");
            CHECK_EQ(params.to_string(), "b=b&c=c");
        }
        {
            whatwg::url_search_params params("a=a&=&b=b&c=c");
            params.del("");
            CHECK_EQ(params.to_string(), "a=a&b=b&c=c");
        }
        {
            whatwg::url_search_params params("a=a&null=null&b=b");
            params.del("null"); // TODO: nullptr
            CHECK_EQ(params.to_string(), "a=a&b=b");
        }
        {
            whatwg::url_search_params params("a=a&undefined=undefined&b=b");
            params.del("undefined"); // TODO: undefined
            CHECK_EQ(params.to_string(), "a=a&b=b");
        }
    }

    SUBCASE("Deleting appended multiple") {
        whatwg::url_search_params params;
        params.append("first", "1"); // TODO: 1
        CHECK_MESSAGE(params.has("first"), "Search params object has name \"first\"");
        CHECK_MESSAGE(param_eq(params.get("first"), "1"), "Search params object has name \"first\" with value \"1\"");
        params.del("first");
        CHECK_FALSE_MESSAGE(params.has("first"), "Search params object has no \"first\" name");
        params.append("first", "1"); // TODO: 1
        params.append("first", "10"); // TODO: 10
        params.del("first");
        CHECK_FALSE_MESSAGE(params.has("first"), "Search params object has no \"first\" name");
    }

#if 0
    // TODO: integrate url_search_params in to URL
    SUBCASE("Deleting all params removes ? from URL") {
        whatwg::URL url("http://example.com/?param1&param2");
        url.searchParams.del("param1");
        url.searchParams.del("param2");
        CHECK_MESSAGE(url.href() == "http://example.com/", "url.href does not have ?");
        CHECK_MESSAGE(url.search() == "", "url.search does not have ?");
    }

    SUBCASE("Removing non-existent param removes ? from URL") {
        whatwg::URL url("http://example.com/?");
        url.searchParams.del("param1");
        CHECK_MESSAGE(url.href() == "http://example.com/", "url.href does not have ?");
        CHECK_MESSAGE(url.search() == "", "url.search does not have ?");
    }
#endif
}

//
// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-foreach.any.js
//
// TODO

//
// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-get.any.js
//
TEST_CASE("urlsearchparams-get.any.js") {
    SUBCASE("Get basics") {
        {
            whatwg::url_search_params params("a=b&c=d");
            CHECK(param_eq(params.get("a"), "b"));
            CHECK(param_eq(params.get("c"), "d"));
            CHECK(params.get("e") == nullptr);
        } {
            whatwg::url_search_params params("a=b&c=d&a=e");
            CHECK(param_eq(params.get("a"), "b"));
        } {
            whatwg::url_search_params params("=b&c=d");
            CHECK(param_eq(params.get(""), "b"));
        } {
            whatwg::url_search_params params("a=&c=d&a=e");
            CHECK(param_eq(params.get("a"), ""));
        }
    }

    SUBCASE("More get() basics") {
        whatwg::url_search_params params("first=second&third&&");
        CHECK_MESSAGE(params.has("first"), "Search params object has name \"first\"");
        CHECK_MESSAGE(param_eq(params.get("first"), "second"), "Search params object has name \"first\" with value \"second\"");
        CHECK_MESSAGE(param_eq(params.get("third"), ""), "Search params object has name \"third\" with the empty value.");
        CHECK_MESSAGE(params.get("fourth") == nullptr, "Search params object has no \"fourth\" name and value.");
    }
}
