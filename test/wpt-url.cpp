// Copyright 2016-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

//
// url library tests
//

#include "upa/url.h"
#include "ddt/DataDrivenTest.hpp"
#include "test-utils.h"
#include "url_cleanup.h"

#include "picojson_util.h"

#include <iostream>
#include <map>
#include <string>


// Test runner

int run_parser_tests(DataDrivenTest& ddt, const char* file_name);
int run_host_parser_tests(DataDrivenTest& ddt, const char* file_name);
int run_idna_v2_tests(DataDrivenTest& ddt, const char* file_name);
int run_setter_tests(DataDrivenTest& ddt, const char* file_name);
int run_percent_encoding_tests(DataDrivenTest& ddt, const char* file_name);

using RunTests = int(*)(DataDrivenTest& ddt, const char* file_name);
int test_from_file(RunTests run_tests, const char* file_name);

int main(int argc, char** argv)
{
    int err = 0;

    // URL web-platform-tests
    err |= test_from_file(run_parser_tests, "wpt/urltestdata.json");
    err |= test_from_file(run_parser_tests, "wpt/urltestdata-javascript-only.json");
    err |= test_from_file(run_host_parser_tests, "wpt/toascii.json");
    err |= test_from_file(run_setter_tests, "wpt/setters_tests.json");
    err |= test_from_file(run_percent_encoding_tests, "wpt/percent-encoding.json");
    if (upa::idna_unicode_version() >= upa::make_unicode_version(13)) {
        // Only the IDNA library that conforms to Unicode 13.0 or later
        // (e.g., ICU 66.1 or later) passes all IdnaTestV2.json tests
        err |= test_from_file(run_idna_v2_tests, "wpt/IdnaTestV2.json");
    }

    // additional tests
    err |= test_from_file(run_parser_tests, "data/my-urltestdata.json");
    err |= test_from_file(run_host_parser_tests, "data/my-toascii.json");
    err |= test_from_file(run_setter_tests, "data/my-setters_tests.json");

    // Free memory
    upa::url_cleanup();

    return err;
}

int test_from_file(RunTests run_tests, const char* file_name)
{
    DataDrivenTest ddt;
    ddt.config_show_passed(false);
    ddt.config_debug_break(true);

    const int res = run_tests(ddt, file_name);

    return res | ddt.result();
}

// URL parser test

class ParserObj : public std::map<std::string, std::string> {
public:
    bool failure = false;
};

//
// https://github.com/web-platform-tests/wpt/blob/master/url/url-constructor.any.js
// https://github.com/web-platform-tests/wpt/blob/master/url/url-origin.any.js
//
void test_parser(DataDrivenTest& ddt, ParserObj& obj)
{
    // https://github.com/web-platform-tests/wpt/blob/master/url/README.md
    // `base`: an absolute URL as a string whose parsing without a base of its own must succeed.
    // `input`: an URL as a string to be parsed with `base` as its base URL.
    const std::string& base = obj["base"];
    const std::string& input = obj["input"];

    const std::string str_case("Parsing <" + input + "> " + (base.empty() ? "without base" : "against <" + base + ">"));

    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        upa::url url;

        bool parse_success = base.empty()
            ? upa::success(url.parse(input))
            : upa::success(url.parse(input, base));

        // check "failure"
        tc.assert_equal(obj.failure, !parse_success, "parse failure");

        // attributes
        if (parse_success && !obj.failure) {
            tc.assert_equal(obj["href"], url.href(), "href");
            auto it_origin = obj.find("origin");
            if (it_origin != obj.end())
                tc.assert_equal(it_origin->second, url.origin(), "origin");
            tc.assert_equal(obj["protocol"], url.protocol(), "protocol");
            tc.assert_equal(obj["username"], url.username(), "username");
            tc.assert_equal(obj["password"], url.password(), "password");
            tc.assert_equal(obj["host"], url.host(), "host");
            tc.assert_equal(obj["hostname"], url.hostname(), "hostname");
            tc.assert_equal(obj["port"], url.port(), "port");
            tc.assert_equal(obj["pathname"], url.pathname(), "pathname");
            tc.assert_equal(obj["search"], url.search(), "search");
            auto it_search_params = obj.find("searchParams");
            if (it_search_params != obj.end())
                tc.assert_equal(it_search_params->second, url.search_params().to_string(), "searchParams");
            tc.assert_equal(obj["hash"], url.hash(), "hash");
        }

        // https://github.com/web-platform-tests/wpt/pull/10955
        // https://github.com/web-platform-tests/wpt/blob/master/url/failure.html
        // If a URL fails to parse with any valid base, it must also fail to parse with no base,
        // i.e. when used as a base URL itself.
        if (obj.failure && !base.empty()) {
            parse_success = upa::success(url.parse(input));
            // check "failure"
            tc.assert_equal(obj.failure, !parse_success, "parse failure WITH NO BASE");
        }

        // Test url::can_parse

        bool can_parse_success = base.empty()
            ? upa::url::can_parse(input)
            : upa::url::can_parse(input, base);

        // check "success"
        tc.assert_equal(!obj.failure, can_parse_success, "can_parse");

        if (obj.failure && !base.empty()) {
            can_parse_success = upa::url::can_parse(input);
            tc.assert_equal(!obj.failure, can_parse_success, "can_parse WITH NO BASE");
        }
    });
}

//
// https://github.com/web-platform-tests/wpt/blob/master/url/toascii.window.js
//
void test_host_parser(DataDrivenTest& ddt, ParserObj& obj)
{
    // Test file format (toascii.json):
    // https://github.com/web-platform-tests/wpt/tree/master/url#toasciijson
    // https://github.com/web-platform-tests/wpt/pull/5976
    static const auto make_url = [](const std::string& host)->std::string {
        std::string str_url("http://");
        str_url += host;
        str_url += "/x";
        return str_url;
    };

    const std::string& input = obj["input"];
    std::string str_case("Parse URL with host: \"" + input + "\"");

    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        const std::string input_url(make_url(input));

        upa::url url;
        const bool parse_success = upa::success(url.parse(input_url));

        // check "failure"
        tc.assert_equal(obj.failure, !parse_success, "parse failure");

        // attributes
        if (parse_success && !obj.failure) {
            const std::string& output = obj["output"];
            const std::string output_url(make_url(output));

            tc.assert_equal(output_url, url.href(), "href");
            tc.assert_equal(output, url.host(), "host");
            tc.assert_equal(output, url.hostname(), "hostname");
            tc.assert_equal("/x", url.pathname(), "pathname");
        }
    });

    str_case = "Set URL.host to: \"" + input + "\"";
    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        upa::url url(make_url("x"), nullptr);

        url.host(input);
        if (!obj.failure) {
            const std::string& output = obj["output"];
            tc.assert_equal(output, url.host(), "host");
        } else {
            tc.assert_equal("x", url.host(), "host");
        }
    });

    str_case = "Set URL.hostname to: \"" + input + "\"";
    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        upa::url url(make_url("x"), nullptr);

        url.hostname(input);
        if (!obj.failure) {
            const std::string& output = obj["output"];
            tc.assert_equal(output, url.hostname(), "hostname");
        } else {
            tc.assert_equal("x", url.hostname(), "hostname");
        }
    });
}

//
// https://github.com/web-platform-tests/wpt/blob/master/url/IdnaTestV2.window.js
//
void test_idna_v2(DataDrivenTest& ddt, ParserObj& obj)
{
    // Test file format (toascii.json):
    // https://github.com/web-platform-tests/wpt/tree/master/url#toasciijson
    // https://github.com/web-platform-tests/wpt/pull/5976
    static const auto make_url = [](const std::string& host) -> std::string {
        std::string str_url("http://");
        str_url += host;
        str_url += "/x";
        return str_url;
    };

    static const auto encodeHostEndingCodePoints = [](const std::string& input) -> std::string {
        if (input.find_first_of(":/?#\\") != input.npos)
            return encodeURIComponent(input);
        return input;
    };

    const std::string& input = obj["input"];
    const std::string& comment = obj["comment"];
    std::string str_case("ToASCII(\"" + input + "\")");
    if (!comment.empty())
        str_case += " " + comment;

    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        const std::string input_url(make_url(encodeHostEndingCodePoints(input)));

        upa::url url;
        const bool parse_success = upa::success(url.parse(input_url));

        // check "failure"
        tc.assert_equal(obj.failure, !parse_success, "parse failure");

        // attributes
        if (parse_success && !obj.failure) {
            const std::string& output = obj["output"];
            const std::string output_url(make_url(output));

            tc.assert_equal(output, url.host(), "host");
            tc.assert_equal(output, url.hostname(), "hostname");
            tc.assert_equal("/x", url.pathname(), "pathname");
            tc.assert_equal(output_url, url.href(), "href");
        }
    });
}

// URL setter test

struct SetterObj {
    explicit SetterObj(std::string setter)
        : m_setter(std::move(setter))
    {}

    std::string m_setter;
    std::string m_href;
    std::string m_new_value;
    std::map<std::string, std::string> m_expected;
};

//
// https://github.com/web-platform-tests/wpt/blob/master/url/url-setters.any.js
//
void test_setter(DataDrivenTest& ddt, SetterObj& obj)
{
    const std::string str_case("URL(\"" + obj.m_href + "\")." + obj.m_setter + "(\"" + obj.m_new_value + "\");");

    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        // url parsing must succeed
        upa::url url(obj.m_href, nullptr);

        // Attributes

        // set value
        if (obj.m_setter == "href") {
            url.href(obj.m_new_value);
        } else if (obj.m_setter == "protocol") {
            url.protocol(obj.m_new_value);
        } else if (obj.m_setter == "username") {
            url.username(obj.m_new_value);
        } else if (obj.m_setter == "password") {
            url.password(obj.m_new_value);
        } else if (obj.m_setter == "host") {
            url.host(obj.m_new_value);
        } else if (obj.m_setter == "hostname") {
            url.hostname(obj.m_new_value);
        } else if (obj.m_setter == "port") {
            url.port(obj.m_new_value);
        } else if (obj.m_setter == "pathname") {
            url.pathname(obj.m_new_value);
        } else if (obj.m_setter == "search") {
            url.search(obj.m_new_value);
        } else if (obj.m_setter == "hash") {
            url.hash(obj.m_new_value);
        }

        // test result
        std::string str_val;
        for (const auto& p : obj.m_expected) {
            if (p.first == "href") str_val = url.href();
            else if (p.first == "origin") str_val = url.origin();
            else if (p.first == "protocol") str_val = url.protocol();
            else if (p.first == "username") str_val = url.username();
            else if (p.first == "password") str_val = url.password();
            else if (p.first == "host") str_val = url.host();
            else if (p.first == "hostname") str_val = url.hostname();
            else if (p.first == "port") str_val = url.port();
            else if (p.first == "pathname") str_val = url.pathname();
            else if (p.first == "search") str_val = url.search();
            else if (p.first == "hash") str_val = url.hash();

            tc.assert_equal(p.second, str_val, p.first);
        }
    });
}

// URL percent encoding test

struct EncodingObj {
    std::string m_input;
    std::map<std::string, std::string> m_output;
};

//
// https://github.com/web-platform-tests/wpt/blob/master/url/percent-encoding.window.js
//
void test_percent_encoding(DataDrivenTest& ddt, EncodingObj& obj)
{
    // URL library supports only UTF-8 encoding
    const std::string& input = obj.m_input;
    const std::string& output = obj.m_output.at("utf-8");

    const std::string str_case = input;

    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        // test percent_encode function
        tc.assert_equal(output, upa::percent_encode(input, upa::special_query_no_encode_set), "percent_encode function");

        // test URL parser
        std::string str_url("https://example.org/?");
        str_url += input;
        str_url += "#";
        str_url += input;
        const upa::url url(str_url);
        tc.assert_equal("#" + output, url.hash(), "url.hash()"); // utf-8
        tc.assert_equal("?" + output, url.search(), "url.search()"); // any encoding
    });
}

// Read samples from JSON files and run tests

namespace {
    enum class TestType {
        UrlParser,
        HostParser,
        IdnaTestV2
    };

    int load_and_run_tests(DataDrivenTest& ddt, TestType test_type, const char* file_name) {
        const auto test_item = [&](const picojson::value& item) {
            // analyze array item
            if (item.is<picojson::object>()) {
                ParserObj obj;

                for (const auto& p : item.get<picojson::object>()) {
                    switch (test_type) {
                    case TestType::UrlParser:
                        // boolean fields
                        if (p.first == "failure") {
                            obj.failure = p.second.evaluate_as_boolean();
                            continue;
                        }
                        if (p.first == "relativeTo") {
                            // skip field intended for browsers only; see:
                            // https://github.com/web-platform-tests/wpt/pull/39203
                            // https://github.com/web-platform-tests/wpt/blob/master/url/failure.html
                            // https://github.com/web-platform-tests/wpt/blob/master/url/resources/a-element.js
                            continue;
                        }
                        break;
                    case TestType::HostParser:
                        if (p.first == "output" && p.second.is<picojson::null>()) {
                            obj.failure = true;
                            continue;
                        }
                        break;
                    case TestType::IdnaTestV2:
                        if (p.first == "input" && p.second.is<std::string>() && p.second.get<std::string>().empty()) {
                            return true; // cannot test empty string input through new URL()
                        }
                        if (p.first == "output" && p.second.is<picojson::null>()) {
                            obj.failure = true;
                            continue;
                        }
                        break;
                    }
                    // do not store null values
                    if (p.second.is<picojson::null>())
                        continue;
                    // string fields
                    if (!p.second.is<std::string>())
                        return false; // error: need string
                    obj[p.first] = p.second.get<std::string>();
                }
                // run item test
                switch (test_type) {
                case TestType::UrlParser:
                    test_parser(ddt, obj);
                    break;
                case TestType::HostParser:
                    test_host_parser(ddt, obj);
                    break;
                case TestType::IdnaTestV2:
                    test_idna_v2(ddt, obj);
                    break;
                }
            } else if (item.is<std::string>()) {
                // comment
                // std::cout << item.get<std::string>() << std::endl;
            } else {
                std::cout << "[ERR: invalid file]" << std::endl;
                return false;
            }
            return true;
        };
        json_util::root_array_context<decltype(test_item)> context{ test_item };

        return json_util::load_file(context, file_name);
    }

} // namespace

int run_parser_tests(DataDrivenTest& ddt, const char* file_name) {
    return load_and_run_tests(ddt, TestType::UrlParser, file_name);
}

int run_host_parser_tests(DataDrivenTest& ddt, const char* file_name) {
    return load_and_run_tests(ddt, TestType::HostParser, file_name);
}

int run_idna_v2_tests(DataDrivenTest& ddt, const char* file_name) {
    return load_and_run_tests(ddt, TestType::IdnaTestV2, file_name);
}

// parses setters_tests.json
int run_setter_tests(DataDrivenTest& ddt, const char* file_name) {
    const auto test_item = [&](const std::string& setter_name, const picojson::value& item) {
        // analyze array item
        if (item.is<picojson::object>()) {
            SetterObj obj(setter_name);

            try {
                const picojson::object& o = item.get<picojson::object>();
                obj.m_href = o.at("href").get<std::string>();
                obj.m_new_value = o.at("new_value").get<std::string>();
                const picojson::object& oexp = o.at("expected").get<picojson::object>();
                for (const auto& pexp : oexp) {
                    if (!pexp.second.is<std::string>())
                        return false; // error: string is expected
                    obj.m_expected[pexp.first] = pexp.second.get<std::string>();
                }
            }
            catch (const std::out_of_range& ex) {
                std::cout << "[ERR:invalid file]: " << ex.what() << std::endl;
                return false;
            }

            test_setter(ddt, obj);
        } else {
            std::cout << "[ERR: invalid file]" << std::endl;
            return false;
        }
        return true;
    };
    const auto filter_name = [](const std::string& name) {
        // skip comment
        return name != "comment";
    };
    json_util::object_array_context<decltype(test_item), decltype(filter_name)> context{test_item, filter_name};

    return json_util::load_file(context, file_name);
}

// parses percent-encoding.json
int run_percent_encoding_tests(DataDrivenTest& ddt, const char* file_name) {
    const auto test_item = [&](const picojson::value& item) {
        // analyze array item
        if (item.is<picojson::object>()) {
            EncodingObj obj;

            try {
                const picojson::object& o = item.get<picojson::object>();
                obj.m_input = o.at("input").get<std::string>();
                const picojson::object& o_output = o.at("output").get<picojson::object>();
                for (const auto& p : o_output) {
                    if (!p.second.is<std::string>())
                        return false; // error: string is expected
                    obj.m_output[p.first] = p.second.get<std::string>();
                }
            }
            catch (const std::out_of_range& ex) {
                std::cout << "[ERR:invalid file]: " << ex.what() << std::endl;
                return false;
            }

            test_percent_encoding(ddt, obj);
        } else if (item.is<std::string>()) {
            // comment
            // std::cout << item.get<std::string>() << std::endl;
        } else {
            std::cout << "[ERR: invalid file]" << std::endl;
            return false;
        }
        return true;
    };
    json_util::root_array_context<decltype(test_item)> context{ test_item };

    return json_util::load_file(context, file_name);
}
