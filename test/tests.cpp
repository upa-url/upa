//
// url library tests
//

#include "url.h"
#include "ddt/DataDrivenTest.hpp"

// https://github.com/kazuho/picojson
# include "picojson/picojson.h"

#include <fstream>
#include <iostream>
#include <map>
#include <string>

// URL parser test

class ParserObj : public std::map<std::string, std::string> {
public:
    ParserObj() : failure(false) {}

    bool failure;
};

void test_parser(DataDrivenTest& ddt, ParserObj& obj)
{
    const std::string& input = obj["input"];
    const std::string& base = obj["base"];

    std::string str_case("<" + input + "> BASE: <" + base + ">");

    ddt.test_case(str_case.c_str(), [&](DataDrivenTest::TestCase& tc) {
        whatwg::url url;
        whatwg::url url_base;

        bool parse_success;
        if (!base.empty()) {
            parse_success = url_base.parse(base, nullptr) == whatwg::url_result::Ok;
            parse_success = parse_success && url.parse(input, &url_base) == whatwg::url_result::Ok;
        } else {
            parse_success = url.parse(input, nullptr) == whatwg::url_result::Ok;
        }

        // check "failure"
        tc.assert_equal(obj.failure, !parse_success, "parse failure");

        // attributes
        if (parse_success && !obj.failure) {
            tc.assert_equal(obj["href"], url.href(), "href");
            auto itOrigin = obj.find("origin");
            if (itOrigin != obj.end())
                tc.assert_equal(itOrigin->second, url.origin(), "origin");
            tc.assert_equal(obj["protocol"], url.protocol(), "protocol");
            tc.assert_equal(obj["username"], url.username(), "username");
            tc.assert_equal(obj["password"], url.password(), "password");
            tc.assert_equal(obj["host"], url.host(), "host");
            tc.assert_equal(obj["hostname"], url.hostname(), "hostname");
            tc.assert_equal(obj["port"], url.port(), "port");
            tc.assert_equal(obj["pathname"], url.pathname(), "pathname");
            tc.assert_equal(obj["search"], url.search(), "search");
            tc.assert_equal(obj["hash"], url.hash(), "hash");
        }
    });
}

void test_host_parser(DataDrivenTest& ddt, ParserObj& obj)
{
    // Test file format:
    // https://github.com/w3c/web-platform-tests/pull/5976
    static const auto make_url = [](const std::string& host)->std::string {
        std::string str_url("http://");
        str_url += host;
        str_url += "/x";
        return str_url;
    };
    
    const std::string& input = obj["input"];
    std::string str_case("URLHost(\"" + input + "\")");

    ddt.test_case(str_case.c_str(), [&](DataDrivenTest::TestCase& tc) {
        std::string input_url(make_url(input));

        whatwg::url url;
        bool parse_success = url.parse(input_url, nullptr) == whatwg::url_result::Ok;

        // check "failure"
        tc.assert_equal(obj.failure, !parse_success, "parse failure");

        // attributes
        if (parse_success && !obj.failure) {
            const auto itOutput = obj.find("output");
            const std::string& output = itOutput != obj.end() ? itOutput->second : input;
            std::string output_url(make_url(output));

            tc.assert_equal(output_url, url.href(), "href");
            tc.assert_equal(output, url.hostname(), "hostname");
        }
    });
}

// URL setter test

class SetterObj {
public:
    SetterObj(const std::string& setter) : m_setter(setter)
    {}

    std::string m_setter;
    std::string m_href;
    std::string m_new_value;
    std::map<std::string, std::string> m_expected;
};

void test_setter(DataDrivenTest& ddt, SetterObj& obj)
{
    std::string str_case("URL(\"" + obj.m_href + "\")." + obj.m_setter + "(\"" + obj.m_new_value + "\");");

    ddt.test_case(str_case.c_str(), [&](DataDrivenTest::TestCase& tc) {
        whatwg::url url;

        bool parse_success = url.parse(obj.m_href, nullptr) == whatwg::url_result::Ok;

        // attributes
        if (parse_success) {
            // set value
            if (obj.m_setter == "protocol") {
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
            for (auto it = obj.m_expected.begin(); it != obj.m_expected.end(); it++) {
                if (it->first == "href") str_val = url.href();
                else if (it->first == "origin") str_val = url.origin();
                else if (it->first == "protocol") str_val = url.protocol();
                else if (it->first == "username") str_val = url.username();
                else if (it->first == "password") str_val = url.password();
                else if (it->first == "host") str_val = url.host();
                else if (it->first == "hostname") str_val = url.hostname();
                else if (it->first == "port") str_val = url.port();
                else if (it->first == "pathname") str_val = url.pathname();
                else if (it->first == "search") str_val = url.search();
                else if (it->first == "hash") str_val = url.hash();
                
                tc.assert_equal(it->second, str_val, it->first.c_str());
            }
        }
    });
}


// Test runner

bool run_parser_tests(DataDrivenTest& ddt, std::ifstream& file);
bool run_host_parser_tests(DataDrivenTest& ddt, std::ifstream& file);
bool run_setter_tests(DataDrivenTest& ddt, std::ifstream& file);

typedef bool(*RunTests)(DataDrivenTest& ddt, std::ifstream& file);

int test_from_file(RunTests run_tests, const char* file_name)
{
    DataDrivenTest ddt;
    ddt.config_show_passed(false);
    ddt.config_debug_break(true);

    std::cout << "========== " << file_name << " ==========\n";
    std::ifstream file(file_name, std::ios_base::in | std::ios_base::binary);
    if (!file.is_open()) {
        std::cerr << "Can't open tests file: " << file_name << std::endl;
        return 4;
    }

    if (!run_tests(ddt, file))
        return 2; // JSON error

    return ddt.result();
}

int main(int argc, char** argv)
{
    int err = 0;

    err |= test_from_file(run_parser_tests, "w3c-tests/urltestdata.json");
    err |= test_from_file(run_parser_tests, "w3c-tests/urltestdata--mano.json");
//  err |= test_from_file(run_parser_tests, "w3c-tests/urltestdata--mano-bandymai.json");

    err |= test_from_file(run_host_parser_tests, "w3c-tests/toascii.json");

    err |= test_from_file(run_setter_tests, "w3c-tests/setters_tests.json");


#if WHATWG_URL_SPEC_ISSUE_303
    err |= test_from_file(run_parser_tests, "w3c-tests/urltestdata--issue-303.json");
#endif

    return err;
}

// Read tests in JSON format

namespace {
    enum class TestType {
        UrlParser,
        HostParser
    };

    // parses urltestdata.json
    class root_context : public picojson::deny_parse_context {
    protected:
        DataDrivenTest& m_ddt;
        TestType m_ttype;
    public:
        root_context(DataDrivenTest& ddt, TestType ttype)
            : m_ddt(ddt)
            , m_ttype(ttype)
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
                ParserObj obj;

                const picojson::object& o = item.get<picojson::object>();
                for (picojson::object::const_iterator it = o.begin(); it != o.end(); it++) {
                    switch (m_ttype) {
                    case TestType::UrlParser:
                        if (it->first == "failure") {
                            obj.failure = it->second.evaluate_as_boolean();
                            continue;
                        }
                        break;
                    case TestType::HostParser:
                        if (it->first == "output" && it->second.is<picojson::null>()) {
                            obj.failure = true;
                            continue;
                        }
                        break;
                    }
                    // string attributes
                    if (!it->second.is<std::string>())
                        return false; // error: need string
                    obj[it->first] = it->second.get<std::string>();
                }
                // run item test
                switch (m_ttype) {
                case TestType::UrlParser:
                    test_parser(m_ddt, obj);
                    break;
                case TestType::HostParser:
                    test_host_parser(m_ddt, obj);
                    break;
                }
            } else if (item.is<std::string>()) {
                // comment
                // std::cout << value.as_string() << std::endl;
            } else {
                std::cout << "[ERR: invalid file]" << std::endl;
                return false;
            }
            return true;
        }
    };

    // parses setters_tests.json
    class root_context2 : public picojson::deny_parse_context {
    protected:
        DataDrivenTest& m_ddt;
        std::string m_setter_name;
    public:
        root_context2(DataDrivenTest& ddt) : m_ddt(ddt) {}

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
                SetterObj obj(m_setter_name);

                try {
                    const picojson::object& o = item.get<picojson::object>();
                    obj.m_href = o.at("href").get<std::string>();
                    obj.m_new_value = o.at("new_value").get<std::string>();
                    const picojson::object& oexp = o.at("expected").get<picojson::object>();
                    for (picojson::object::const_iterator itexp = oexp.begin(); itexp != oexp.end(); itexp++) {
                        if (!itexp->second.is<std::string>())
                            return false; // klaida: reikia string
                        obj.m_expected[itexp->first] = itexp->second.get<std::string>();
                    }
                }
                catch (const std::out_of_range& ex) {
                    std::cout << "[ERR:invalid file]: " << ex.what() << std::endl;
                    return false;
                }

                test_setter(m_ddt, obj);
            } else {
                std::cout << "[ERR: invalid file]" << std::endl;
                return false;
            }
            return true;
        }

        // object only as root
        bool parse_object_start() { return true; }

        template <typename Iter> bool parse_object_item(picojson::input<Iter>& in, const std::string& name) {
            if (name == "comment") {
                // skip
                picojson::null_parse_context nullctx;
                return picojson::_parse(nullctx, in);
            } else {
                m_setter_name = name;
                // parse array
                return picojson::_parse(*this, in);
            }
        }
    };
}

template <typename Context>
bool run_some_tests(Context &ctx, std::ifstream& file) {
    std::string err;

    // for unformatted reading use std::istreambuf_iterator
    // http://stackoverflow.com/a/17776228/3908097
    picojson::_parse(ctx, std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), &err);

    if (!err.empty()) {
        std::cerr << err << std::endl;
        return false;
    }
    return true;
}

bool run_parser_tests(DataDrivenTest& ddt, std::ifstream& file) {
    root_context ctx(ddt, TestType::UrlParser);
    return run_some_tests(ctx, file);
}

bool run_host_parser_tests(DataDrivenTest& ddt, std::ifstream& file) {
    root_context ctx(ddt, TestType::HostParser);
    return run_some_tests(ctx, file);
}

bool run_setter_tests(DataDrivenTest& ddt, std::ifstream& file) {
    root_context2 ctx(ddt);
    return run_some_tests(ctx, file);
}
