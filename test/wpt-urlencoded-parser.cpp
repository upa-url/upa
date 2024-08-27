// Copyright 2016-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url.h"
#include "url_cleanup.h"

// https://github.com/kazuho/picojson
#include "picojson_util.h"

#include <iostream>
#include <string>

// for DataDrivenTest
template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& os,
    const upa::url_search_params::name_value_pair& v);
#include "ddt/DataDrivenTest.hpp"

//
// Testing code and data based on
// https://github.com/web-platform-tests/wpt/blob/master/url/urlencoded-parser.any.js
// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-sort.any.js
//

int test_from_file(const char* file_name, bool sort = false);

int main(int argc, char** argv)
{
    int err = 0;

    err |= test_from_file("data/urlencoded-parser.json");
    err |= test_from_file("data/urlsearchparams-sort.json", true);

    // NOTE: "Sorting non-existent params removes ? from URL" test based on
    // urlsearchparams-sort.any.js is in the wpt-url_search_params.cpp

    // Free memory
    upa::url_cleanup();

    return err;
}

// Test classes

struct TestObj {
    std::string m_input;
    upa::url_search_params::name_value_list m_output;
};

// url_search_params::name_value_pair stream output function for DataDrivenTest
template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& os,
    const upa::url_search_params::name_value_pair& v)
{
    return os << "[\"" << v.first << "\", \"" << v.second << "\"]";
}

static void do_assert_equal(DataDrivenTest::TestCase& tc,
    const upa::url_search_params::name_value_list& expected,
    const upa::url_search_params& sparams)
{
    const size_t n_sparams = sparams.size();
    const size_t n_expected = expected.size();
    tc.assert_equal(n_expected, n_sparams, "parameters count");

    if (n_sparams == n_expected) {
        int nparam = 0;
        auto itexp = expected.begin();
        for (auto p : sparams) {
            tc.assert_equal(*itexp, p, "parameter " + std::to_string(nparam));
            ++itexp;
            ++nparam;
        }
    }
}

// https://github.com/web-platform-tests/wpt/blob/master/url/urlencoded-parser.any.js

void test_urlencoded_parser(DataDrivenTest& ddt, const TestObj& obj) {
    std::string str_case("url_search_params constructed with: \"" + obj.m_input + "\"");

    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        upa::url_search_params sparams(obj.m_input);
        do_assert_equal(tc, obj.m_output, sparams);
    });
}

// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-sort.any.js

void test_urlsearchparams_sort(DataDrivenTest& ddt, const TestObj& obj) {
    std::string str_case("Parse and sort: \"" + obj.m_input + "\"");
    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        upa::url_search_params sparams(obj.m_input);
        sparams.sort();
        do_assert_equal(tc, obj.m_output, sparams);
    });

    str_case = "URL parse and sort: \"" + obj.m_input + "\"";
    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        upa::url url("?" + obj.m_input, "https://example/");

        url.search_params().sort();

        upa::url_search_params sparams(url.search());
        do_assert_equal(tc, obj.m_output, sparams);
    });
}

// Read data file and run tests from it

int test_from_file(const char* file_name, bool sort)
{
    DataDrivenTest ddt;
    ddt.config_show_passed(false);
    ddt.config_debug_break(false);

    const auto test_item = [&](const picojson::value& item) {
        // analyze array item
        if (item.is<picojson::object>()) {
            TestObj obj;

            try {
                const picojson::object& o = item.get<picojson::object>();
                obj.m_input = o.at("input").get<std::string>();
                const picojson::array& output = o.at("output").get<picojson::array>();
                for (auto it = output.begin(); it != output.end(); ++it) {
                    const picojson::array& pair = it->get<picojson::array>();
                    obj.m_output.emplace_back(pair[0].get<std::string>(), pair[1].get<std::string>());
                }
            }
            catch (const std::exception& ex) {
                std::cout << "[ERR: invalid file]: " << ex.what() << std::endl;
                return false;
            }

            if (sort)
                test_urlsearchparams_sort(ddt, obj);
            else
                test_urlencoded_parser(ddt, obj);
        } else if (item.is<std::string>()) {
            // comment
            // std::cout << value.as_string() << std::endl;
        } else {
            std::cout << "[ERR: invalid file]" << std::endl;
            return false;
        }
        return true;
    };
    json_util::root_array_context<decltype(test_item)> context{ test_item };

    const int res = json_util::load_file(context, file_name);

    return res | ddt.result();
}
