// Copyright 2016-2021 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url.h"
#include "url_search_params.h"
#include "url_cleanup.h"

// https://github.com/kazuho/picojson
#include "picojson/picojson.h"

#include <fstream>
#include <iostream>
#include <string>

// for DataDrivenTest
template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& os,
    const whatwg::url_search_params::name_value_pair& v);
#include "ddt/DataDrivenTest.hpp"

//
// Testing code and data based on
// https://github.com/web-platform-tests/wpt/blob/master/url/urlencoded-parser.any.js
// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-sort.any.js
//
// Last checked for updates: 2021-06-08
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
    whatwg::url_cleanup();

    return err;
}

// Test classes

struct TestObj {
    std::string m_input;
    whatwg::url_search_params::name_value_list m_output;
};

// url_search_params::name_value_pair stream output function for DataDrivenTest
template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& os,
    const whatwg::url_search_params::name_value_pair& v)
{
    return os << "[\"" << v.first << "\", \"" << v.second << "\"]";
}

static void do_assert_equal(DataDrivenTest::TestCase& tc,
    const whatwg::url_search_params::name_value_list& expected,
    const whatwg::url_search_params& sparams)
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
        whatwg::url_search_params sparams(obj.m_input);
        do_assert_equal(tc, obj.m_output, sparams);
    });
}

// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-sort.any.js

void test_urlsearchparams_sort(DataDrivenTest& ddt, const TestObj& obj) {
    std::string str_case("Parse and sort: \"" + obj.m_input + "\"");
    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        whatwg::url_search_params sparams(obj.m_input);
        sparams.sort();
        do_assert_equal(tc, obj.m_output, sparams);
    });

    str_case = "URL parse and sort: \"" + obj.m_input + "\"";
    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        whatwg::url url("?" + obj.m_input, "https://example/");

        url.searchParams().sort();

        whatwg::url_search_params sparams(url.search());
        do_assert_equal(tc, obj.m_output, sparams);
    });
}


class root_context : public picojson::deny_parse_context {
protected:
    DataDrivenTest& m_ddt;
    bool m_sort;
public:
    root_context(DataDrivenTest& ddt, bool sort)
        : m_ddt(ddt)
        , m_sort(sort)
    {}

    // array only as root
    bool parse_array_start() { return true; }
    bool parse_array_stop(std::size_t) { return true; }

    template <typename Iter> bool parse_array_item(picojson::input<Iter>& in, std::size_t) {
        picojson::value item;

        // parse the array item
        picojson::default_parse_context ctx(&item);
        if (!picojson::_parse(ctx, in))
            return false;

        // analyze array item
        if (item.is<picojson::object>()) {
            const picojson::object& o = item.get<picojson::object>();

            TestObj obj;
            obj.m_input = o.at("input").get<std::string>();
            const picojson::array& output = o.at("output").get<picojson::array>();
            for (auto it = output.begin(); it != output.end(); ++it) {
                const picojson::array& pair = it->get<picojson::array>();
                obj.m_output.emplace_back(pair[0].get<std::string>(), pair[1].get<std::string>());
            }

            if (m_sort)
                test_urlsearchparams_sort(m_ddt, obj);
            else
                test_urlencoded_parser(m_ddt, obj);
        } else if (item.is<std::string>()) {
            // comment
            // std::cout << value.as_string() << std::endl;
        } else {
            std::cout << "[ERR: invalid file]" << std::endl;
            return false;
        }
        return true;
    }

    // deny object as root
    bool parse_object_start() { return false; }
    bool parse_object_stop() { return false; }
};

// Read data file and run tests from it

int test_from_file(const char* file_name, bool sort)
{
    DataDrivenTest ddt;
    ddt.config_show_passed(false);
    ddt.config_debug_break(false);

    try {
        std::cout << "========== " << file_name << " ==========\n";
        std::ifstream file(file_name, std::ios_base::in | std::ios_base::binary);
        if (!file.is_open()) {
            std::cerr << "Can't open tests file: " << file_name << std::endl;
            return 4;
        }

        std::string err;
        // for unformatted reading use std::istreambuf_iterator
        // http://stackoverflow.com/a/17776228/3908097
        root_context ctx(ddt, sort);
        picojson::_parse(ctx, std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), &err);
        if (!err.empty()) {
            std::cerr << err << std::endl;
            return 2; // JSON error
        }
    }
    catch (std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return 3;
    }

    return ddt.result();
}
