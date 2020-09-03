#include "url.h"
#include "url_search_params.h"
#include "doctest-main.h"
#include "test-utils.h"
#include <map>

#if defined(__has_include)

// The old macro __cpp_coroutines is still used in Visual Studio 2019 16.7
#if defined(__cpp_impl_coroutine) || defined(__cpp_coroutines)
#if __has_include(<experimental/generator>)
# include <experimental/generator>
# define TEST_COROUTINE_GENERATOR
using std::experimental::generator;
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
}

TEST_CASE_TEMPLATE_INVOKE(test_iterables, char, wchar_t, char16_t, char32_t);
#ifdef __cpp_char8_t
TEST_CASE_TEMPLATE_INVOKE(test_iterables, char8_t);
#endif


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


// Test url::searchParams()

TEST_CASE("url::searchParams()") {
    whatwg::url url("http://h/p?a=A");
    auto& params = url.searchParams();

    INFO("url::search(...) -> url::searchParams()");

    // initial
    CHECK(list_eq(params, pairs_list_t<std::string>{ {"a", "A"} }));

    // replace search
    url.search("b=B");
    CHECK(list_eq(params, pairs_list_t<std::string>{ {"b", "B"} }));

    // clear search
    url.search("");
    CHECK(params.empty());

    INFO("url::searchParams() -> url::search()");

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
