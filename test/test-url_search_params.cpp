// Copyright 2016-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url.h"
#include "doctest-main.h"
#include "test-utils.h"
#include <map>
#include <sstream>

#if defined(__has_include)

#if __has_include(<version>)
# include <version>
#endif

// Use libc++ or the Microsoft C++ Standard Library to test code that
// contains coroutines and ranges when compiling with Clang
#if !defined(__clang__) || defined(_LIBCPP_VERSION) || defined(_MSC_VER)
#if defined(__cpp_lib_generator)
# include <generator>
# define TEST_COROUTINE_GENERATOR
#endif
#if defined(__cpp_lib_ranges)
# include <ranges>
# define TEST_RANGES
#endif
#endif

#endif // defined(__has_include)

// Conversion to doctest String
// https://github.com/doctest/doctest/blob/master/doc/markdown/stringification.md

namespace doctest {
    template<> struct StringMaker<upa::url_search_params::value_type> {
        static String convert(const upa::url_search_params::value_type& value) {
            std::string str{ "[\"" };
            str.append(value.first);
            str.append("\", \"");
            str.append(value.second);
            str.append("\"]");
            return { str.data(), static_cast<String::size_type>(str.length()) };
        }
    };
}

// Iterable containers tests

#define TEST_ITERABLES_DATA {    \
        {{ 'a' }, { 'a', 'a' }}, \
        {{ 'b' }, { 'b', 'b' }}  \
    }

#define TEST_SEARCH_STR "?a=aa&b=bb"


TEST_CASE_TEMPLATE_DEFINE("Various string pairs iterable containers", CharT, test_iterables) {
    using string_t = std::basic_string<CharT>;

    const pairs_list_t<std::string> output = TEST_ITERABLES_DATA;

    SUBCASE("array") {
        const std::pair<string_t, string_t> arr_pairs[] = TEST_ITERABLES_DATA;
        upa::url_search_params params(arr_pairs);
        CHECK(list_eq(params, output));
    }
    SUBCASE("std::initializer_list") {
        const pairs_list_t<string_t> lst_pairs = TEST_ITERABLES_DATA;
        upa::url_search_params params(lst_pairs);
        CHECK(list_eq(params, output));
    }
    SUBCASE("std::map") {
        const std::map<string_t, string_t> map_pairs(TEST_ITERABLES_DATA);
        upa::url_search_params params(map_pairs);
        CHECK(list_eq(params, output));
    }
    SUBCASE("std::vector") {
        const std::vector<std::pair<string_t, string_t>> vec_pairs(TEST_ITERABLES_DATA);
        upa::url_search_params params(vec_pairs);
        CHECK(list_eq(params, output));
    }

#ifdef TEST_COROUTINE_GENERATOR
    SUBCASE("coroutine generator") {
        auto pairs_gen = []() -> std::generator<std::pair<string_t, string_t>> {
            const pairs_list_t<string_t> lst_pairs = TEST_ITERABLES_DATA;
            for (const auto& p : lst_pairs)
                co_yield p;
        };

        upa::url_search_params params(pairs_gen());
        CHECK(list_eq(params, output));
    }
#endif

#ifdef TEST_RANGES
    SUBCASE("range as input to url_search_params ctor") {
        const pairs_list_t<string_t> lst_pairs = TEST_ITERABLES_DATA;
        const upa::url_search_params params(std::ranges::subrange(lst_pairs.begin(), lst_pairs.end()));
        CHECK(list_eq(params, output));
    }
    SUBCASE("url_search_params object as range") {
        const pairs_list_t<string_t> lst_pairs = TEST_ITERABLES_DATA;
        const upa::url_search_params params(lst_pairs);
        constexpr auto leave_a = [](const auto& param) {
            return param.first == "a";
        };
        auto result = params | std::views::filter(leave_a);
        auto expect = output | std::views::filter(leave_a);
        CHECK(std::ranges::equal(result, expect));
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
        const upa::url_search_params params;

        CHECK(params.empty());
        CHECK(params.size() == 0);
        CHECK(params.to_string() == "");
    }
    SUBCASE("copy constructor from const") {
        const upa::url_search_params params(lst_pairs);
        CHECK(list_eq(params, lst_pairs));

        upa::url_search_params paramsC(params);
        CHECK(list_eq(paramsC, lst_pairs));
    }
    SUBCASE("copy constructor") {
        upa::url_search_params params(lst_pairs);
        CHECK(list_eq(params, lst_pairs));

        upa::url_search_params paramsC(params);
        CHECK(list_eq(paramsC, lst_pairs));
    }
    SUBCASE("move constructor") {
        upa::url_search_params params(lst_pairs);
        CHECK(list_eq(params, lst_pairs));

        upa::url_search_params paramsM(std::move(params));
        CHECK(list_eq(paramsM, lst_pairs));
    }
}

// Assignment

TEST_CASE("url_search_params assignment") {
    const pairs_list_t<std::string> lst_pairs = TEST_ITERABLES_DATA;

    SUBCASE("copy assignment") {
        const upa::url_search_params params(lst_pairs);
        upa::url_search_params paramsC("x=y");
        REQUIRE(list_eq(params, lst_pairs));

        paramsC = params;
        CHECK(list_eq(paramsC, lst_pairs));
    }

    SUBCASE("move assignment") {
        upa::url_search_params params(lst_pairs);
        upa::url_search_params paramsM("x=y");
        REQUIRE(list_eq(params, lst_pairs));

        paramsM = std::move(params);
        CHECK(list_eq(paramsM, lst_pairs));
    }

    SUBCASE("safe move assignment") {
        upa::url_search_params params(lst_pairs);
        upa::url_search_params paramsM("x=y");
        REQUIRE(list_eq(params, lst_pairs));

        paramsM.safe_assign(std::move(params));
        CHECK(list_eq(paramsM, lst_pairs));
    }
}

TEST_CASE("url_search_params assignment to url::search_params()") {
    const pairs_list_t<std::string> lst_pairs = TEST_ITERABLES_DATA;

    SUBCASE("copy assignment") {
        const upa::url_search_params params(lst_pairs);
        REQUIRE(list_eq(params, lst_pairs));

        upa::url url("http://h/?y=x");
        url.search_params() = params;

        CHECK(list_eq(url.search_params(), lst_pairs));
        CHECK(url.search() == TEST_SEARCH_STR);
    }

    SUBCASE("safe move assignment") {
        upa::url_search_params params(lst_pairs);
        REQUIRE(list_eq(params, lst_pairs));

        upa::url url("http://h/?y=x");
        url.search_params().safe_assign(std::move(params));

        CHECK(list_eq(url.search_params(), lst_pairs));
        CHECK(url.search() == TEST_SEARCH_STR);
    }
}

// Operations

TEST_CASE("swap(url_search_params&, url_search_params&)") {
    const std::string str_params_1 = "a=1&b=2&c=3";
    const std::string str_params_2 = "d=4";

    upa::url_search_params params_1(str_params_1);
    upa::url_search_params params_2(str_params_2);

    using std::swap;

    swap(params_1, params_2);
    CHECK(params_1.to_string() == str_params_2);
    CHECK(params_2.to_string() == str_params_1);
}

TEST_CASE("url_search_params::parse") {
    upa::url_search_params params;

    params.parse("a=b");
    CHECK(params.to_string() == "a=b");

    params.parse("?c=d");
    CHECK(params.to_string() == "c=d");
}

TEST_CASE("url_search_params::remove...") {
    SUBCASE("url_search_params::remove") {
        upa::url_search_params params("a=a&a=A&b=b&b=B");

        CHECK(params.remove("a") == 2);
        CHECK(params.to_string() == "b=b&b=B");

        CHECK(params.remove("b", "B") == 1);
        CHECK(params.to_string() == "b=b");
    }
    SUBCASE("url_search_params::remove_if") {
        upa::url_search_params params("a=a&a=A&b=b&b=B");

        CHECK(params.remove_if([](upa::url_search_params::value_type& item) {
            return item.first == item.second;
        }) == 2);
        CHECK(params.to_string() == "a=A&b=B");
    }
    SUBCASE("url::search_params") {
        upa::url url("http://h?a&b=B");

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
        INFO(std::string{val.comment});
        upa::url_search_params params(val.input);
        params.sort();
        CHECK(list_eq(params, val.output));
    }
}


// Test url::search_params()

TEST_CASE("url::search_params()") {
    upa::url url("http://h/p?a=A");
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

TEST_CASE("url::search_params() of rvalue url object") {
    const char* str_url = "http://h/" TEST_SEARCH_STR;
    // expected values
    const std::pair<std::string, std::string> arr_pairs[] = TEST_ITERABLES_DATA;
    // TODO: use std::size(arr_pairs) if C++17
    const std::size_t count = std::distance(std::begin(arr_pairs), std::end(arr_pairs));

    SUBCASE("temporary url") {
        std::size_t ind = 0;
        for (auto& item : upa::url(str_url).search_params()) {
            REQUIRE(ind < count);
            CHECK(item == arr_pairs[ind]);
            ++ind;
        }
        CHECK(ind == count);
    }

    SUBCASE("moved url") {
        upa::url url{ str_url };
        url.search_params();

        std::size_t ind = 0;
        for (auto& item : std::move(url).search_params()) {
            REQUIRE(ind < count);
            CHECK(item == arr_pairs[ind]);
            ++ind;
        }
        CHECK(ind == count);
    }
}

TEST_CASE("moved url::search_params()") {
    const char* str_url = "http://h/" TEST_SEARCH_STR;

    upa::url url{ str_url };
    // usp.url_ptr_ must be nullptr after move construction,
    // i.e. must not be connected to url
    upa::url_search_params usp{ std::move(url.search_params()) };
    const auto str = url.to_string();
    usp.append("p", "priv");
    CHECK(str == url.to_string());
}

TEST_CASE("url::search_params() and url::operator=(const url&)") {
    // test copy assignment to url with initialized url_search_params
    upa::url url_ca("http://dest/");
    auto& ca_search_params = url_ca.search_params();
    ca_search_params.append("ca", "CA");
    CHECK(url_ca.search() == "?ca=CA");

    // copy assign url with not initialized url_search_params
    upa::url url("http://src/?a=A");
    url_ca = url;
    REQUIRE(std::addressof(ca_search_params) == std::addressof(url_ca.search_params()));
    CHECK(ca_search_params.to_string() == "a=A");

    // copy assign url with initialized url_search_params
    url.search_params().clear();
    url.search_params().append("b", "B");
    url_ca = url;
    REQUIRE(std::addressof(ca_search_params) == std::addressof(url_ca.search_params()));
    CHECK(ca_search_params.to_string() == "b=B");
}

TEST_CASE("url::search_params() and url::url(url&&)") {
    // test move constructor from url with initialized url_search_params
    upa::url url("http://example.org/");
    url.search_params().append("a", "A");
    CHECK(url.search() == "?a=A");

    // move constructor
    upa::url url_m(std::move(url));
    url_m.search_params().append("m", "M");
    CHECK(url_m.search() == "?a=A&m=M");
}

TEST_CASE("url::search_params() and url::operator=(url&&)") {
    // test move assignment from url with initialized url_search_params
    upa::url url("http://example.org/");
    url.search_params().append("a", "A");
    CHECK(url.search() == "?a=A");

    // move assignment
    upa::url url_m;
    url_m = std::move(url);
    url_m.search_params().append("m", "M");
    CHECK(url_m.search() == "?a=A&m=M");
}

TEST_CASE("url::search_params() and url::safe_assign(url&&)") {
    // test safe_assign(...) to url with initialized url_search_params
    upa::url url_sa("http://dest/");
    auto& sa_search_params = url_sa.search_params();
    sa_search_params.append("sa", "SA");
    CHECK(url_sa.search() == "?sa=SA");

    // safe_assign url with not initialized url_search_params
    url_sa.safe_assign(upa::url("http://src/?a=A"));
    REQUIRE(std::addressof(sa_search_params) == std::addressof(url_sa.search_params()));
    CHECK(sa_search_params.to_string() == "a=A");

    // safe_assign url with initialized url_search_params
    upa::url url("http://src/");
    url.search_params().append("b", "B");
    url_sa.safe_assign(std::move(url));
    REQUIRE(std::addressof(sa_search_params) == std::addressof(url_sa.search_params()));
    CHECK(sa_search_params.to_string() == "b=B");
}

TEST_CASE("url::search_params() and url::href(...)") {
    // test href setter on url with initialized url_search_params
    upa::url url("http://dest/");
    auto& search_params = url.search_params();
    search_params.append("hr", "HR");
    CHECK(url.search() == "?hr=HR");

    url.href("http://href/?a=A");
    REQUIRE(std::addressof(search_params) == std::addressof(url.search_params()));
    CHECK(search_params.to_string() == "a=A");
}

TEST_CASE("url::search_params() and url::search(...)") {
    upa::url url("http://h/p");
    auto& params = url.search_params();

    url.search("??a=b&c=d");
    CHECK(url.search() == "??a=b&c=d");
    CHECK(params.to_string() == "%3Fa=b&c=d");
}

TEST_CASE("url::search_params() and url::clear()") {
    upa::url url("http://h/p?a=A&b=B");
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

// Test operator<<

TEST_CASE("url_search_params operator<<") {
    const auto input = "a=b&c=d";

    SUBCASE("url_search_params") {
        std::ostringstream sout;
        sout << upa::url_search_params{ input };
        CHECK(sout.str() == input);
    }
    SUBCASE("url::search_params()") {
        upa::url u{ "http://example.com/" };
        u.search(input);
        std::ostringstream sout;
        sout << u.search_params();
        CHECK(sout.str() == input);
    }
}
