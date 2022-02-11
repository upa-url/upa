// Copyright 2016-2022 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url.h"
#include "url_search_params.h"
#include "doctest-main.h"
#include "test-utils.h"
#include <map>

#if defined(__has_include)

#if __has_include(<version>)
# include <version>
#endif

#if defined(__cpp_impl_coroutine)
#if __has_include(<experimental/generator>)
# include <experimental/generator>
# define TEST_COROUTINE_GENERATOR
using std::experimental::generator;
#endif
#endif

// Clang has partial ranges support in the libc++ 13
#if !defined(__clang__) || defined(_LIBCPP_VERSION)
// Visual Studio 2019 16.8 (_MSC_VER = 1928) and concepts are sufficient to test ranges support
#if defined(__cpp_lib_ranges) || (_MSC_VER >= 1928 && defined(__cpp_lib_concepts)) || (defined(__clang__) && _LIBCPP_VERSION >= 13000)
#if __has_include(<ranges>)
# include <ranges>
# define TEST_RANGES
#endif
#endif
#endif

#endif // defined(__has_include)


// Iterable containers tests

#define TEST_ITERABLES_DATA {    \
        {{ 'a' }, { 'a', 'a' }}, \
        {{ 'b' }, { 'b', 'b' }}  \
    }

TEST_CASE_TEMPLATE_DEFINE("Various string pairs iterable containers", CharT, test_iterables) {
    using string_t = std::basic_string<CharT>;

    const pairs_list_t<std::string> output = TEST_ITERABLES_DATA;

    SUBCASE("array") {
        const std::pair<string_t, string_t> arr_pairs[] = TEST_ITERABLES_DATA;
        whatwg::url_search_params params(arr_pairs);
        CHECK(list_eq(params, output));
    }
    SUBCASE("std::initializer_list") {
        const pairs_list_t<string_t> lst_pairs = TEST_ITERABLES_DATA;
        whatwg::url_search_params params(lst_pairs);
        CHECK(list_eq(params, output));
    }
    SUBCASE("std::map") {
        const std::map<string_t, string_t> map_pairs(TEST_ITERABLES_DATA);
        whatwg::url_search_params params(map_pairs);
        CHECK(list_eq(params, output));
    }
    SUBCASE("std::vector") {
        const std::vector<std::pair<string_t, string_t>> vec_pairs(TEST_ITERABLES_DATA);
        whatwg::url_search_params params(vec_pairs);
        CHECK(list_eq(params, output));
    }

#ifdef TEST_COROUTINE_GENERATOR
    SUBCASE("coroutine generator") {
        auto pairs_gen = []() -> generator<std::pair<string_t, string_t>> {
            const pairs_list_t<string_t> lst_pairs = TEST_ITERABLES_DATA;
            for (const auto& p : lst_pairs)
                co_yield p;
        };

        whatwg::url_search_params params(pairs_gen());
        CHECK(list_eq(params, output));
    }
#endif

#ifdef TEST_RANGES
    SUBCASE("ranges") {
        const pairs_list_t<string_t> lst_pairs = TEST_ITERABLES_DATA;
        whatwg::url_search_params params(std::ranges::subrange(lst_pairs.begin(), lst_pairs.end()));
        CHECK(list_eq(params, output));
    }
#endif
}

TEST_CASE_TEMPLATE_INVOKE(test_iterables, char, wchar_t, char16_t, char32_t);
#ifdef __cpp_char8_t
TEST_CASE_TEMPLATE_INVOKE(test_iterables, char8_t);
#endif


// Constructors

TEST_CASE("url_search_params constructors") {
    const pairs_list_t<std::string> lst_pairs = TEST_ITERABLES_DATA;

    SUBCASE("default constructor") {
        const whatwg::url_search_params params;

        CHECK(params.empty());
        CHECK(params.size() == 0);
        CHECK(params.to_string() == "");
    }
    SUBCASE("copy constructor from const") {
        const whatwg::url_search_params params(lst_pairs);
        CHECK(list_eq(params, lst_pairs));

        whatwg::url_search_params paramsC(params);
        CHECK(list_eq(paramsC, lst_pairs));
    }
    SUBCASE("copy constructor") {
        whatwg::url_search_params params(lst_pairs);
        CHECK(list_eq(params, lst_pairs));

        whatwg::url_search_params paramsC(params);
        CHECK(list_eq(paramsC, lst_pairs));
    }
    SUBCASE("move constructor") {
        whatwg::url_search_params params(lst_pairs);
        CHECK(list_eq(params, lst_pairs));

        whatwg::url_search_params paramsM(std::move(params));
        CHECK(list_eq(paramsM, lst_pairs));
    }
}

// Operations

TEST_CASE("url_search_params::parse") {
    whatwg::url_search_params params;

    params.parse("a=b");
    CHECK(params.to_string() == "a=b");

    params.parse("?c=d");
    CHECK(params.to_string() == "c=d");
}

TEST_CASE("url_search_params::remove...") {
    SUBCASE("url_search_params::remove") {
        whatwg::url_search_params params("a=a&a=A&b=b&b=B");

        CHECK(params.remove("a") == 2);
        CHECK(params.to_string() == "b=b&b=B");

        CHECK(params.remove("b", "B") == 1);
        CHECK(params.to_string() == "b=b");
    }
    SUBCASE("url_search_params::remove_if") {
        whatwg::url_search_params params("a=a&a=A&b=b&b=B");

        CHECK(params.remove_if([](whatwg::url_search_params::value_type& item) {
            return item.first == item.second;
        }) == 2);
        CHECK(params.to_string() == "a=A&b=B");
    }
    SUBCASE("url::search_params") {
        whatwg::url url("http://h?a&b=B");

        CHECK(url.search_params().remove("A") == 0);
        CHECK(url.search() == "?a&b=B");

        CHECK(url.search_params().remove("b") == 1);
        CHECK(url.search() == "?a=");
    }
}

// Sort test

TEST_CASE("url_search_params::sort()") {
    struct {
        const char* comment;
        pairs_list_t<std::u32string> input;
        pairs_list_t<std::string> output;
    } lst[] = {
        {
            "Sort U+104 before U+41104",
            {{U"a\U00041104", U"2"}, {U"a\u0104", U"1"}},
            {{mk_string(u8"a\u0104"), "1"}, {mk_string(u8"a\U00041104"), "2"}},
        }, {
            "Sort U+105 before U+41104",
            {{U"a\U00041104", U"2"}, {U"a\u0105", U"1"}},
            {{mk_string(u8"a\u0105"), "1"}, {mk_string(u8"a\U00041104"), "2"}},
        }, {
            "Sort U+D7FF before U+10000",
            {{U"a\U00010000", U"2"}, {U"a\uD7FF", U"1"}},
            {{mk_string(u8"a\uD7FF"), "1"}, {mk_string(u8"a\U00010000"), "2"}},
        }, {
            "Sort U+10FFFF before U+E000",
            {{U"a\uE000", U"2"}, {U"a\U0010FFFF", U"1"}},
            {{mk_string(u8"a\U0010FFFF"), "1"}, {mk_string(u8"a\uE000"), "2"}},
        }, {
            "Sort U+10FFFE before U+10FFFF",
            {{U"a\U0010FFFF", U"2"}, {U"a\U0010FFFE", U"1"}},
            {{mk_string(u8"a\U0010FFFE"), "1"}, {mk_string(u8"a\U0010FFFF"), "2"}},
        }
    };
    for (const auto& val : lst) {
        INFO(val.comment);
        whatwg::url_search_params params(val.input);
        params.sort();
        CHECK(list_eq(params, val.output));
    }
}


// Test url::search_params()

TEST_CASE("url::search_params()") {
    whatwg::url url("http://h/p?a=A");
    auto& params = url.search_params();

    INFO("url::search(...) -> url::search_params()");

    // initial
    CHECK(list_eq(params, pairs_list_t<std::string>{ {"a", "A"} }));

    // replace search
    url.search("b=B");
    CHECK(list_eq(params, pairs_list_t<std::string>{ {"b", "B"} }));

    // clear search
    url.search("");
    CHECK(params.empty());

    INFO("url::search_params() -> url::search()");

    // add parameters
    params.append("c", "C");
    params.append("d", "D");
    params.append("e", "E");
    CHECK(url.search() == "?c=C&d=D&e=E");

    // delete parameter
    params.del("d");
    CHECK(url.search() == "?c=C&e=E");

    // set parameters
    params.set("c", "CC");
    params.set("d", "DD");
    CHECK(url.search() == "?c=CC&e=E&d=DD");

    // clear parameters
    params.clear();
    CHECK(url.search() == "");
}

TEST_CASE("url::search_params() and url::url(url&&)") {
    // test move constructor from url with initialized url_search_params  
    whatwg::url url("http://example.org/");
    url.search_params().append("a", "A");
    CHECK(url.search() == "?a=A");

    // move constructor
    whatwg::url url_m(std::move(url));
    url_m.search_params().append("m", "M");
    CHECK(url_m.search() == "?a=A&m=M");
}

TEST_CASE("url::search_params() and url::operator=(url&&)") {
    // test move assignment from url with initialized url_search_params  
    whatwg::url url("http://example.org/");
    url.search_params().append("a", "A");
    CHECK(url.search() == "?a=A");

    // move assignment
    whatwg::url url_m;
    url_m = std::move(url);
    url_m.search_params().append("m", "M");
    CHECK(url_m.search() == "?a=A&m=M");
}

TEST_CASE("url::search_params() and url::search(...)") {
    whatwg::url url("http://h/p");
    auto& params = url.search_params();

    url.search("??a=b&c=d");
    CHECK(url.search() == "??a=b&c=d");
    CHECK(params.to_string() == "%3Fa=b&c=d");
}

TEST_CASE("url::search_params() and url::clear()") {
    whatwg::url url("http://h/p?a=A&b=B");
    auto& params = url.search_params();

    CHECK_FALSE(url.empty());
    CHECK_FALSE(params.empty());
    CHECK(list_eq(params, pairs_list_t<std::string>{ {"a", "A"}, { "b", "B" } }));
    CHECK(params.size() == 2);

    url.clear();

    CHECK(url.href() == "");
    CHECK(url.search() == "");
    CHECK(url.empty());
    CHECK(params.empty());
    CHECK(params.size() == 0);
}
