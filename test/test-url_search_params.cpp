#include "url.h"
#include "url_search_params.h"
#include "doctest-main.h"
#include <algorithm>

// Tests based on "urlsearchparams-*.any.js" files from
// https://github.com/web-platform-tests/wpt/tree/master/url

template <class T>
static bool param_eq(const std::string* pval, const T& value) {
    return pval != nullptr && *pval == value;
}

template <class List, class T>
static bool list_eq(const List& val, std::initializer_list<T> lst) {
#if 0 // TODO: if >= C++14
    return std::equal(std::begin(val), std::end(val), std::begin(lst), std::end(lst))
#else
    return
        std::size(val) == std::size(lst) &&
        std::equal(std::begin(val), std::end(val), std::begin(lst));
#endif
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
        } {
            whatwg::url_search_params params("");
            CHECK_EQ(params.to_string(), "");
        } {
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
        } {
            whatwg::url_search_params params("a=b&c");
            CHECK_MESSAGE(params.has("a"), "Search params object has name \"a\"");
            CHECK_MESSAGE(params.has("c"), "Search params object has name \"c\"");
        } {
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
        } {
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
        } {
            whatwg::url_search_params params("a b=c");
            CHECK(param_eq(params.get("a b"), "c"));
        }
    }

    SUBCASE("Parse %20") {
        {
            whatwg::url_search_params params("a=b%20c");
            CHECK(param_eq(params.get("a"), "b c"));
        } {
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
        } {
            whatwg::url_search_params params(std::string("a\0b=c", 5));
            CHECK(param_eq(params.get(std::string("a\0b", 3)), "c"));
        }
    }

    SUBCASE("Parse %00") {
        {
            whatwg::url_search_params params("a=b%00c");
            CHECK(param_eq(params.get("a"), std::string("b\0c", 3)));
        } {
            whatwg::url_search_params params("a%00b=c");
            CHECK(param_eq(params.get(std::string("a\0b", 3)), "c"));
        }
    }

    // Unicode Character 'COMPOSITION SYMBOL' (U+2384)
    SUBCASE(u8"Parse \u2384") {
        {
            whatwg::url_search_params params(u8"a=b\u2384");
            CHECK(param_eq(params.get("a"), u8"b\u2384"));
        } {
            whatwg::url_search_params params(u8"a\u2384b=c");
            CHECK(param_eq(params.get(u8"a\u2384b"), "c"));
        }
    }

    // Unicode Character 'COMPOSITION SYMBOL' (U+2384)
    SUBCASE("Parse %e2%8e%84") {
        {
            whatwg::url_search_params params("a=b%e2%8e%84");
            CHECK(param_eq(params.get("a"), u8"b\u2384"));
        } {
            whatwg::url_search_params params("a%e2%8e%84b=c");
            CHECK(param_eq(params.get(u8"a\u2384b"), "c"));
        }
    }

    // Unicode Character 'PILE OF POO' (U+1F4A9)
    SUBCASE(u8"Parse \U0001F4A9") {
        {
            whatwg::url_search_params params(u8"a=b\U0001F4A9");
            CHECK(param_eq(params.get("a"), u8"b\U0001F4A9"));
        } {
            whatwg::url_search_params params(u8"a\U0001F4A9b=c");
            CHECK(param_eq(params.get(u8"a\U0001F4A9b"), "c"));
        }
    }

    // Unicode Character 'PILE OF POO' (U+1F4A9)
    SUBCASE("Parse %f0%9f%92%a9") {
        {
            whatwg::url_search_params params("a=b%f0%9f%92%a9c");
            CHECK(param_eq(params.get("a"), u8"b\U0001F4A9c"));
        } {
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
        } {
            whatwg::url_search_params params("a=a&b=b&a=a&c=c");
            params.del("a");
            CHECK_EQ(params.to_string(), "b=b&c=c");
        } {
            whatwg::url_search_params params("a=a&=&b=b&c=c");
            params.del("");
            CHECK_EQ(params.to_string(), "a=a&b=b&c=c");
        } {
            whatwg::url_search_params params("a=a&null=null&b=b");
            params.del("null"); // TODO: nullptr
            CHECK_EQ(params.to_string(), "a=a&b=b");
        } {
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

//
// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-getall.any.js
//
TEST_CASE("urlsearchparams-getall.any.js") {
    SUBCASE("getAll() basics") {
        {
            whatwg::url_search_params params("a=b&c=d");
            CHECK(list_eq(params.getAll("a"), { "b" }));
            CHECK(list_eq(params.getAll("c"), { "d" }));
            CHECK(params.getAll("e").empty()); // empty list
        } {
            whatwg::url_search_params params("a=b&c=d&a=e");
            CHECK(list_eq(params.getAll("a"), { "b", "e" }));
        } {
            whatwg::url_search_params params("=b&c=d");
            CHECK(list_eq(params.getAll(""), { "b" }));
        } {
            whatwg::url_search_params params("a=&c=d&a=e");
            CHECK(list_eq(params.getAll("a"), { "", "e" }));
        }
    }

    SUBCASE("getAll() multiples") {
        whatwg::url_search_params params("a=1&a=2&a=3&a");
        CHECK_MESSAGE(params.has("a"), "Search params object has name \"a\"");
        auto matches = params.getAll("a");
        CHECK_MESSAGE(matches.size() == 4, "Search params object has values for name \"a\"");
        CHECK_MESSAGE(list_eq(matches, { "1", "2", "3", "" }), "Search params object has expected name \"a\" values");
        params.set("a", "one");
        CHECK_MESSAGE(param_eq(params.get("a"), "one"), "Search params object has name \"a\" with value \"one\"");
        matches = params.getAll("a");
        CHECK_MESSAGE(matches.size() == 1, "Search params object has values for name \"a\"");
        CHECK_MESSAGE(list_eq(matches, { "one" }), "Search params object has expected name \"a\" values");
    }
}

//
// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-has.any.js
//
TEST_CASE("urlsearchparams-has.any.js") {
    SUBCASE("Has basics") {
        {
            whatwg::url_search_params params("a=b&c=d");
            CHECK(params.has("a"));
            CHECK(params.has("c"));
            CHECK_FALSE(params.has("e"));
        } {
            whatwg::url_search_params params("a=b&c=d&a=e");
            CHECK(params.has("a"));
        } {
            whatwg::url_search_params params("=b&c=d");
            CHECK(params.has(""));
        } {
            whatwg::url_search_params params("null=a");
            CHECK(params.has("null")); // todo: nullptr
        }
    }

    SUBCASE("has() following delete()") {
        whatwg::url_search_params params("a=b&c=d&&");
        params.append("first", "1"); // TODO: 1
        params.append("first", "2"); // TODO: 2
        CHECK_MESSAGE(params.has("a"), "Search params object has name \"a\"");
        CHECK_MESSAGE(params.has("c"), "Search params object has name \"c\"");
        CHECK_MESSAGE(params.has("first"), "Search params object has name \"first\"");
        CHECK_FALSE_MESSAGE(params.has("d"), "Search params object has no name \"d\"");
        params.del("first");
        CHECK_FALSE_MESSAGE(params.has("first"), "Search params object has no name \"first\"");
    }
}

//
// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-set.any.js
//
TEST_CASE("urlsearchparams-set.any.js") {
    SUBCASE("Set basics") {
        {
            whatwg::url_search_params params("a=b&c=d");
            params.set("a", "B");
            CHECK_EQ(params.to_string(), "a=B&c=d");
        } {
            whatwg::url_search_params params("a=b&c=d&a=e");
            params.set("a", "B");
            CHECK_EQ(params.to_string(), "a=B&c=d");
            params.set("e", "f");
            CHECK_EQ(params.to_string(), "a=B&c=d&e=f");
        }
    }

    SUBCASE("URLSearchParams.set") {
        whatwg::url_search_params params("a=1&a=2&a=3");
        CHECK_MESSAGE(params.has("a"), "Search params object has name \"a\"");
        CHECK_MESSAGE(param_eq(params.get("a"), "1"), "Search params object has name \"a\" with value \"1\"");
        params.set("first", "4"); // TODO: 4
        CHECK_MESSAGE(params.has("a"), "Search params object has name \"a\"");
        CHECK_MESSAGE(param_eq(params.get("a"), "1"), "Search params object has name \"a\" with value \"1\"");
        params.set("a", "4"); // TODO: 4
        CHECK_MESSAGE(params.has("a"), "Search params object has name \"a\"");
        CHECK_MESSAGE(param_eq(params.get("a"), "4"), "Search params object has name \"a\" with value \"4\"");
    }
}

//
// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-stringifier.any.js
//
TEST_CASE("urlsearchparams-stringifier.any.js") {
    SUBCASE("Serialize space") {
        whatwg::url_search_params params;
        params.append("a", "b c");
        CHECK_EQ(params.to_string(), "a=b+c");
        params.del("a");
        params.append("a b", "c");
        CHECK_EQ(params.to_string(), "a+b=c");
    }

    SUBCASE("Serialize empty value") {
        whatwg::url_search_params params;
        params.append("a", "");
        CHECK_EQ(params.to_string(), "a=");
        params.append("a", "");
        CHECK_EQ(params.to_string(), "a=&a=");
        params.append("", "b");
        CHECK_EQ(params.to_string(), "a=&a=&=b");
        params.append("", "");
        CHECK_EQ(params.to_string(), "a=&a=&=b&=");
        params.append("", "");
        CHECK_EQ(params.to_string(), "a=&a=&=b&=&=");
    }

    SUBCASE("Serialize empty name") {
        whatwg::url_search_params params;
        params.append("", "b");
        CHECK_EQ(params.to_string(), "=b");
        params.append("", "b");
        CHECK_EQ(params.to_string(), "=b&=b");
    }

    SUBCASE("Serialize empty name and value") {
        whatwg::url_search_params params;
        params.append("", "");
        CHECK_EQ(params.to_string(), "=");
        params.append("", "");
        CHECK_EQ(params.to_string(), "=&=");
    }

    SUBCASE("Serialize +") {
        whatwg::url_search_params params;
        params.append("a", "b+c");
        CHECK_EQ(params.to_string(), "a=b%2Bc");
        params.del("a");
        params.append("a+b", "c");
        CHECK_EQ(params.to_string(), "a%2Bb=c");
    }

    SUBCASE("Serialize =") {
        whatwg::url_search_params params;
        params.append("=", "a");
        CHECK_EQ(params.to_string(), "%3D=a");
        params.append("b", "=");
        CHECK_EQ(params.to_string(), "%3D=a&b=%3D");
    }

    SUBCASE("Serialize &") {
        whatwg::url_search_params params;
        params.append("&", "a");
        CHECK_EQ(params.to_string(), "%26=a");
        params.append("b", "&");
        CHECK_EQ(params.to_string(), "%26=a&b=%26");
    }

    SUBCASE("Serialize *-._") {
        whatwg::url_search_params params;
        params.append("a", "*-._");
        CHECK_EQ(params.to_string(), "a=*-._");
        params.del("a");
        params.append("*-._", "c");
        CHECK_EQ(params.to_string(), "*-._=c");
    }

    SUBCASE("Serialize %") {
        whatwg::url_search_params params;
        params.append("a", "b%c");
        CHECK_EQ(params.to_string(), "a=b%25c");
        params.del("a");
        params.append("a%b", "c");
        CHECK_EQ(params.to_string(), "a%25b=c");
    }

    SUBCASE("Serialize \\0") {
        whatwg::url_search_params params;
        params.append("a", std::string("b\0c", 3));
        CHECK_EQ(params.to_string(), "a=b%00c");
        params.del("a");
        params.append(std::string("a\0b", 3), "c");
        CHECK_EQ(params.to_string(), "a%00b=c");
    }

    // Unicode Character 'PILE OF POO' (U+1F4A9)
    SUBCASE(u8"Serialize \U0001F4A9") {
        whatwg::url_search_params params;
        params.append("a", u8"b\U0001F4A9c");
        CHECK_EQ(params.to_string(), "a=b%F0%9F%92%A9c");
        params.del("a");
        params.append(u8"a\U0001F4A9b", "c");
        CHECK_EQ(params.to_string(), "a%F0%9F%92%A9b=c");
    }

    SUBCASE("URLSearchParams.toString") {
        {
            whatwg::url_search_params params("a=b&c=d&&e&&");
            CHECK_EQ(params.to_string(), "a=b&c=d&e=");
        } {
            whatwg::url_search_params params("a = b &a=b&c=d%20");
            CHECK_EQ(params.to_string(), "a+=+b+&a=b&c=d+");
        } {
            // The lone "=" _does_ survive the roundtrip.
            whatwg::url_search_params params("a=&a=b");
            CHECK_EQ(params.to_string(), "a=&a=b");
        }
    }

    SUBCASE("URLSearchParams connected to URL") {
#if 0
        // TODO: URL with searchParams
        whatwg::URL url("http://www.example.com/?a=b,c");
        auto& params = url.searchParams;

        CHECK_EQ(url.to_string(), "http://www.example.com/?a=b,c");
        CHECK_EQ(params.to_string(), "a=b%2Cc");

        params.append("x", "y");

        CHECK_EQ(url.to_string(), "http://www.example.com/?a=b%2Cc&x=y");
        CHECK_EQ(params.to_string(), "a=b%2Cc&x=y");
#else
        whatwg::url url;
        REQUIRE(whatwg::success(url.parse("http://www.example.com/?a=b,c", nullptr)));
        whatwg::url_search_params params(url.search());

        CHECK_EQ(url.to_string(), "http://www.example.com/?a=b,c");
        CHECK_EQ(params.to_string(), "a=b%2Cc");

        params.append("x", "y");
        url.search(params.to_string());

        CHECK_EQ(url.to_string(), "http://www.example.com/?a=b%2Cc&x=y");
        CHECK_EQ(params.to_string(), "a=b%2Cc&x=y");
#endif
    }
}
