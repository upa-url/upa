// Copyright 2023-2026 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#include "upa/url.h"
#include "upa/urlpattern.h"
#ifdef UPA_TEST_WITH_STD_REGEX
# include "upa/regex_engine_std.h"
#else
# include "upa/regex_engine_srell.h"
#endif

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

template <class K, class V>
inline std::string vout(const std::unordered_map<K, V>& m);
template <class T>
inline std::string vout(const std::optional<T>& o);
constexpr std::string_view vout(std::nullopt_t);

#include "ddt/DataDrivenTest.hpp"
#include "picojson_util.h"

#define TEST_DEBUG 0

using namespace std::string_literals;
using namespace std::string_view_literals;

#ifdef UPA_TEST_WITH_STD_REGEX
using urlpattern = upa::urlpattern<upa::regex_engine_std>;
#else
using urlpattern = upa::urlpattern<upa::regex_engine_srell>;
#endif

// This is a workaround for Clang versions earlier than 19. These versions of Clang
// do not deduce the template argument type of the std::initializer_list from a list
// of objects (using CTAD), so we specify this type explicitly.
// See: https://stackoverflow.com/q/55205176
using string_view_pairs = std::initializer_list<std::pair<std::string_view, std::string_view>>;

// -----------------------------------------------------------------------------
// Helper functions

static picojson::value json_parse(std::string_view strv) {
    picojson::value v;
    picojson::parse(v, strv.begin(), strv.end(), nullptr);
    return v;
}

// String conversions for DataDrivenTest

template <class K, class V>
inline std::string vout(const std::unordered_map<K, V>& m) {
    bool first = true;
    std::string out("{");
    for (const auto& [key, val] : m) {
        if (first) first = false; else out += ", ";
        out += '\"';
        out += key;
        out += "\": ";
        if (val) {
            out += '\"';
            out += *val;
            out += '\"';
        } else {
            out += "null";
        }
    }
    out += '}';
    return out;
}

template <class T>
inline std::string vout(const std::optional<T>& o) {
    return o ? std::string{ *o } : "null";
}

constexpr std::string_view vout(std::nullopt_t) {
    return "null"sv;
}

// String conversions for doctest

namespace doctest {
    template<class T> struct StringMaker<std::optional<T>> {
        static String convert(const std::optional<T>& oval) {
            if (oval) {
                std::string sval{ *oval };
                return { sval.data(), static_cast<String::size_type>(sval.size()) };
            }
            return "nullopt";
        }
    };
    template<> struct StringMaker<std::nullopt_t> {
        static String convert(std::nullopt_t) {
            return "nullopt";
        }
    };
} // namespace doctest

// -----------------------------------------------------------------------------
// https://github.com/web-platform-tests/wpt/blob/master/urlpattern/urlpattern-constructor.html

TEST_SUITE("urlpattern_constructor") {
    TEST_CASE("Test unclosed token") {
        CHECK_THROWS_AS(urlpattern{ upa::url("https://example.org/%(").to_string() },
            upa::urlpattern_error);
        CHECK_THROWS_AS(urlpattern{ upa::url("https://example.org/%((").to_string() },
            upa::urlpattern_error);
        CHECK_THROWS_AS(urlpattern{ "(\\" }, upa::urlpattern_error);
    }
    // The following JavaScript test case is skipped because C++ does not have
    // an equivalent of the value undefined:
    // test(() => {
    //   new URLPattern(undefined, undefined);
    // }, `Test constructor with undefined`);
}

// -----------------------------------------------------------------------------
// https://github.com/web-platform-tests/wpt/blob/master/urlpattern/resources/urlpattern-hasregexpgroups-tests.js

template <class T>
upa::urlpattern_init create_urlpattern_init(const T& obj) {
    upa::urlpattern_init init;
    for (const auto& [key, val] : obj)
        init.set(key, val);
    return init;
}

int wpt_urlpattern_hasregexpgroups_tests() {
    DataDrivenTest ddt;
#if TEST_DEBUG
    ddt.config_debug_break(true);
#endif

    std::cout << "========== urlpattern has regexp groups ==========\n";

    ddt.test_case("urlpattern has_regexp_groups() tests", [&](DataDrivenTest::TestCase& tc) {
        tc.assert_equal(false, urlpattern{}.has_regexp_groups(), "match-everything pattern");

        for (std::string_view component : { "protocol", "username", "password", "hostname", "port", "pathname", "search", "hash" }) {
            tc.assert_equal(false, urlpattern{ create_urlpattern_init(string_view_pairs{
                std::pair{component, "*"sv} }) }.has_regexp_groups(), "wildcard in "s.append(component));
            tc.assert_equal(false, urlpattern{ create_urlpattern_init(string_view_pairs{
                std::pair{component, ":foo"sv} }) }.has_regexp_groups(), "segment wildcard in "s.append(component));
            tc.assert_equal(false, urlpattern{ create_urlpattern_init(string_view_pairs{
                std::pair{component, ":foo?"sv} }) }.has_regexp_groups(), "optional segment wildcard in "s.append(component));
            tc.assert_equal(true, urlpattern{ create_urlpattern_init(string_view_pairs{
                std::pair{component, ":foo(hi)"sv} }) }.has_regexp_groups(), "named regexp group in "s.append(component));
            tc.assert_equal(true, urlpattern{ create_urlpattern_init(string_view_pairs{
                std::pair{component, "(hi)"sv} }) }.has_regexp_groups(), "anonymous regexp group in "s.append(component));
            if (component != "protocol"sv && component != "port"sv) {
                // These components are more narrow in what they accept in any case.
                tc.assert_equal(false, urlpattern{ create_urlpattern_init(string_view_pairs{
                    std::pair{component, "a-{:hello}-z-*-a"sv} }) }.has_regexp_groups(),
                    "wildcards mixed in with fixed text and wildcards in "s.append(component));
                tc.assert_equal(true, urlpattern{ create_urlpattern_init(string_view_pairs{
                    std::pair{component, "a-(hi)-z-(lo)-a"sv} }) }.has_regexp_groups(),
                    "regexp groups mixed in with fixed text and wildcards in "s.append(component));
            }
        }

        tc.assert_equal(false, urlpattern{ create_urlpattern_init(string_view_pairs{
            std::pair{"pathname"sv, "/a/:foo/:baz?/b/*"sv}})}.has_regexp_groups(), "complex pathname with no regexp");
        tc.assert_equal(true, urlpattern{ create_urlpattern_init(string_view_pairs{
            std::pair{"pathname"sv, "/a/:foo/:baz([a-z]+)?/b/*"sv}}) }.has_regexp_groups(), "complex pathname with regexp");
    });

    return ddt.result();
}

// -----------------------------------------------------------------------------
// https://github.com/web-platform-tests/wpt/blob/master/urlpattern/resources/urlpatterntests.js

struct exec_args {
    exec_args(const picojson::array& input_arr);

    std::variant<std::string, upa::urlpattern_init> arg_;
    std::optional<std::string> base_;
};

urlpattern create_urlpattern(const picojson::array& pattern_arr);
bool urlpattern_test(const urlpattern& self, const exec_args& input);
std::optional<upa::urlpattern_result> urlpattern_exec(const urlpattern& self, const exec_args& input);


auto get_component(const upa::url& url, std::string_view name) -> std::string_view {
    if (name == "protocol"sv) return url.protocol();
    if (name == "username"sv) return url.username();
    if (name == "password"sv) return url.password();
    if (name == "host"sv) return url.host();
    if (name == "hostname"sv) return url.hostname();
    if (name == "port"sv) return url.port();
    if (name == "pathname"sv) return url.pathname();
    if (name == "search"sv) return url.search();
    if (name == "hash"sv) return url.hash();
    throw std::out_of_range("not url componnent"); // TODO: message
};

auto get_component(const urlpattern& urlpt, std::string_view name) -> std::string_view {
    if (name == "protocol"sv) return urlpt.get_protocol();
    if (name == "username"sv) return urlpt.get_username();
    if (name == "password"sv) return urlpt.get_password();
    if (name == "hostname"sv) return urlpt.get_hostname();
    if (name == "port"sv) return urlpt.get_port();
    if (name == "pathname"sv) return urlpt.get_pathname();
    if (name == "search"sv) return urlpt.get_search();
    if (name == "hash"sv) return urlpt.get_hash();
    throw std::out_of_range("not urlpattern componnent"); // TODO: message
};

const upa::urlpattern_component_result& get_component_result(const upa::urlpattern_result& result, std::string_view name) {
    if (name == "protocol"sv) return result.protocol;
    if (name == "username"sv) return result.username;
    if (name == "password"sv) return result.password;
    if (name == "hostname"sv) return result.hostname;
    if (name == "port"sv) return result.port;
    if (name == "pathname"sv) return result.pathname;
    if (name == "search"sv) return result.search;
    if (name == "hash"sv) return result.hash;
    throw std::out_of_range("not urlpattern_result componnent"); // TODO: message
}

template<class StrT>
void assert_object_equals(DataDrivenTest::TestCase& tc, const picojson::value& expected_obj,
    const upa::urlpattern_component_result& res, const StrT& name)
{
    upa::urlpattern_component_result expected;

    expected.input = expected_obj.get<picojson::object>().at("input").get<std::string>();
    for (const auto& [key, val] : expected_obj.get<picojson::object>().at("groups").get<picojson::object>()) {
        std::optional<std::string> value;
        if (val.template is<picojson::null>())
            value = std::nullopt;
        else
            value = val.template get<std::string>();
        expected.groups.emplace(key, value);
    }
    tc.assert_equal(expected.input, res.input, std::string{name} + " - input");
    tc.assert_equal(expected.groups, res.groups, std::string{name} + " - groups");
}


int wpt_urlpatterntests(const std::filesystem::path& file_name) {
    DataDrivenTest ddt;
#if TEST_DEBUG
    ddt.config_debug_break(true);
#endif

    static const std::string_view kComponents[] = {
        "protocol"sv,
        "username"sv,
        "password"sv,
        "hostname"sv,
        "port"sv,
        "password"sv,
        "pathname"sv,
        "search"sv,
        "hash"sv
    };

    static const std::unordered_map<std::string_view, std::vector<std::string_view>> EARLIER_COMPONENTS = {
        {"protocol"sv, {}},
        {"hostname"sv, {"protocol"sv}},
        {"port"sv, {"protocol"sv, "hostname"sv}},
        {"username"sv, {}},
        {"password"sv, {}},
        {"pathname"sv, {"protocol"sv, "hostname"sv, "port"sv}},
        {"search"sv, {"protocol"sv, "hostname"sv, "port"sv, "pathname"sv}},
        {"hash"sv, {"protocol"sv, "hostname"sv, "port"sv, "pathname"sv, "search"sv}}
    };

    static constexpr auto get_prop = [](const picojson::object& obj, std::string_view name) -> const picojson::value* {
        const auto it = obj.find(std::string{ name });
        if (it != obj.end())
            return &it->second;
        return nullptr;
    };

    static constexpr auto has_prop = [](const picojson::object& obj, std::string_view name) -> bool {
#ifdef UPA_CPP_20
        return obj.contains(std::string{ name });
#else
        return obj.find(std::string{ name }) != obj.end();
#endif
    };

    static constexpr auto includes = [](const picojson::array& arr, std::string_view name) -> bool {
        return std::any_of(arr.begin(), arr.end(), [&name](const picojson::value& val) {
            return val.get<std::string>() == name;
        });
    };

    static constexpr auto some = [](auto arr, auto pred) {
        return std::any_of(arr.begin(), arr.end(), pred);
    };

    // Load & run tests
    json_util::root_array_context context{ [&](const picojson::value& item) {
        static const picojson::value empty_object_value{ picojson::object_type, false };

        if (item.is<picojson::object>()) {
            try {
                const picojson::object& entry = item.get<picojson::object>();
                const auto entry_pattern = entry.at("pattern");
                const auto* entry_inputs = get_prop(entry, "inputs");
                const auto* entry_exactly_empty_components = get_prop(entry, "exactly_empty_components");
                const auto* entry_expected_obj = get_prop(entry, "expected_obj");
                const auto* entry_expected_match = get_prop(entry, "expected_match");

                std::string test_case_name{"Pattern: "};
                test_case_name.append(entry_pattern.serialize());
                if (entry_inputs) {
                    test_case_name.append(" Inputs: ");
                    test_case_name.append(entry_inputs->serialize());
                }

                ddt.test_case(test_case_name, [&](DataDrivenTest::TestCase& tc) {
                    if (entry_expected_obj && entry_expected_obj->is<std::string>() && entry_expected_obj->get<std::string>() == "error") {
                        tc.assert_throws<upa::urlpattern_error>([&]() {
                            create_urlpattern(entry_pattern.get<picojson::array>());
                        }, "URLPattern() constructor");
                        return;
                    }

                    const auto pattern = create_urlpattern(entry_pattern.get<picojson::array>());

                    // If the expected_obj property is not present we will automatically
                    // fill it with the most likely expected values.
                    if (!entry_expected_obj)
                        entry_expected_obj = &empty_object_value;

                    // The compiled URLPattern object should have a property for each
                    // component exposing the compiled pattern string.
                    for (auto component : kComponents) {
                        // If the test case explicitly provides an expected pattern string,
                        // then use that.  This is necessary in cases where the original
                        // construction pattern gets canonicalized, etc.
                        const auto* expected = get_prop(entry_expected_obj->get<picojson::object>(), component);
                        std::string expected_str;
                        if (expected) expected_str = expected->get<std::string>();

                        // If there is no explicit expected pattern string, then compute
                        // the expected value based on the URLPattern constructor args.
                        if (expected == nullptr) {
                            // First determine if there is a baseURL present in the pattern
                            // input.  A baseURL can be the source for many component patterns.
                            std::optional<upa::url> baseURL{};
                            if (entry_pattern.get<picojson::array>().size() > 0 &&
                                entry_pattern.get<picojson::array>()[0].is<picojson::object>() &&
                                has_prop(entry_pattern.get<picojson::array>()[0].get<picojson::object>(), "baseURL"))
                                baseURL.emplace(get_prop(entry_pattern.get<picojson::array>()[0].get<picojson::object>(), "baseURL")->get<std::string>());
                            else if (entry_pattern.get<picojson::array>().size() > 1 &&
                                entry_pattern.get<picojson::array>()[1].is<std::string>())
                                baseURL.emplace(entry_pattern.get<picojson::array>()[1].get<std::string>());

                            // We automatically populate the expected pattern string using
                            // the following options in priority order:
                            //
                            //  1. If the original input explicitly provided a pattern, then
                            //     echo that back as the expected value.
                            //  2. If an "earlier" component is specified, then a wildcard
                            //     will be used rather than inheriting from the base URL.
                            //  3. If the baseURL exists and provides a component value then
                            //     use that for the expected pattern.
                            //  4. Otherwise fall back on the default pattern of `*` for an
                            //     empty component pattern.
                            //
                            //  Note that username and password are never inherited, and will only
                            //  need to match if explicitly specified.
                            if (entry_exactly_empty_components &&
                                includes(entry_exactly_empty_components->get<picojson::array>(), component)) {
                                expected_str = "";
                            } else if (entry_pattern.get<picojson::array>().size() > 0 &&
                                entry_pattern.get<picojson::array>()[0].is<picojson::object>() &&
                                has_prop(entry_pattern.get<picojson::array>()[0].get<picojson::object>(), component)) {
                                expected_str = get_prop(entry_pattern.get<picojson::array>()[0].get<picojson::object>(), component)->get<std::string>();
                            } else if (entry_pattern.get<picojson::array>().size() > 0 &&
                                entry_pattern.get<picojson::array>()[0].is<picojson::object>() &&
                                some(EARLIER_COMPONENTS.at(component), [&entry_pattern](auto c) -> bool {
                                    // Check whether obj has property c
                                    const auto obj = entry_pattern.get<picojson::array>()[0].get<picojson::object>();
#ifdef UPA_CPP_20
                                    return obj.contains(std::string{ c });
#else
                                    return obj.find(std::string{ c }) != obj.end();
#endif
                                })) {
                                expected_str = "*";
                            } else if (baseURL && component != "username"sv && component != "password"sv) {
                                auto base_value = get_component(*baseURL, component);
                                // Unfortunately some URL() getters include separator chars; e.g.
                                // the trailing `:` for the protocol.  Strip those off if necessary.
                                if (component == "protocol"sv) {
                                    if (base_value.length() > 0) // MANO
                                        base_value.remove_suffix(1);
                                } else if (component == "search"sv || component == "hash"sv) {
                                    if (base_value.length() > 0) // MANO
                                        base_value.remove_prefix(1);
                                }
                                expected_str = base_value;
                            } else {
                                expected_str = "*";
                            }
                        }
                        // Finally, assert that the compiled object property matches the
                        // expected property.
                        tc.assert_equal(expected_str, get_component(pattern, component),
                            std::string{ "compiled pattern property " } + std::string{ component });
                    }

                    // entry.inputs
                    exec_args entry_inputs_args(entry_inputs->get<picojson::array>());
                    if (entry_expected_match && entry_expected_match->is<std::string>() && entry_expected_match->get<std::string>() == "error") {
                        // pattern.test
                        tc.assert_throws<upa::urlpattern_error>([&]() {
                            urlpattern_test(pattern, entry_inputs_args);
                        }, "test() result");

                        // pattern.exec
                        tc.assert_throws<upa::urlpattern_error>([&]() {
                            urlpattern_exec(pattern, entry_inputs_args);
                        }, "exec() result");

                        return;
                    }

                    // First, validate the test() method by converting the expected result to
                    // a truthy value.
                    tc.assert_equal(
                        entry_expected_match && !entry_expected_match->is<picojson::null>(),
                        urlpattern_test(pattern, entry_inputs_args),
                        "test() result");

                    // Next, start validating the exec() method.
                    const auto exec_result = urlpattern_exec(pattern, entry_inputs_args);

                    // On a failed match exec() returns null.
                    if (!entry_expected_match || !entry_expected_match->is<picojson::object>()) {
                        // MANO: nullptr, picojson::null, std::nullopt
                        tc.assert_equal(
                            entry_expected_match == nullptr || entry_expected_match->is<picojson::null>(),
                            exec_result == std::nullopt,
                            "exec() failed match result");
                        return;
                    }
                    // MANO:
                    if (!exec_result) {
                        tc.assert_equal(
                            entry_expected_match == nullptr || entry_expected_match->is<picojson::null>(),
                            exec_result == std::nullopt,
                            "exec() failed match result");
                        return;
                    }

                    const auto* entry_expected_match_inputs = get_prop(entry_expected_match->get<picojson::object>(), "inputs");
                    if (!entry_expected_match_inputs)
                        entry_expected_match_inputs = entry_inputs;

                    // Next verify the result.input is correct.  This may be a structured
                    // URLPatternInit dictionary object or a URL string.
                    tc.assert_equal(
                        entry_expected_match_inputs->get<picojson::array>().size(),
                        exec_result->inputs.size(), // MANO: ? :
                        "exec() result.inputs.length");

                    if (exec_result->inputs.size() == entry_expected_match_inputs->get<picojson::array>().size()) {
                        for (int i = 0; i < exec_result->inputs.size(); ++i) {
                            const auto input = exec_result->inputs[i];
                            const auto expected_input = entry_expected_match_inputs->get<picojson::array>()[i];

                            std::string val_name("exec() result.inputs[");
                            val_name.append(std::to_string(i));
                            val_name.append("]");

                            if (expected_input.is<std::string>()) {
                                if (std::holds_alternative<std::string_view>(input))
                                    tc.assert_equal(expected_input.get<std::string>(), std::get<std::string_view>(input), val_name);
                                else
                                    tc.failure() << val_name << " is not string" << std::endl;
                                continue;
                            }
                            if (expected_input.is<picojson::object>()) {
                                if (std::holds_alternative<const upa::urlpattern_init*>(input)) {
                                    /// auto expected_init = expected_input.get<picojson::object>();
                                    const auto* input_init = std::get<const upa::urlpattern_init*>(input);

                                    if (input_init) {
                                        for (auto component : kComponents) {
                                            const auto* expected_val_ptr = get_prop(expected_input.get<picojson::object>(), component);
                                            std::optional<std::string> expected_val;
                                            if (expected_val_ptr)
                                                expected_val = expected_val_ptr->get<std::string>();
                                            auto input_val = input_init->get(component);

                                            // `exec() result.inputs[${i}][${component}]`
                                            std::string val_name_comp = val_name + "[" + std::string{ component } + "]";
                                            tc.assert_equal(expected_val, input_val, val_name_comp);
                                        }
                                    } else
                                        tc.failure() << val_name << " is null urlpattern_init" << std::endl;
                                } else
                                    tc.failure() << val_name << " is not urlpattern_init" << std::endl;
                            } // else // TODO: error in test file
                        }
                    }

                    // Next we will compare the URLPatternComponentResult for each of these
                    // expected components.
                    for (auto component : kComponents) {
                        picojson::value expected_obj;
                        const auto* expected_obj_ptr = get_prop(entry_expected_match->get<picojson::object>(), component);
                        if (expected_obj_ptr)
                            expected_obj = *expected_obj_ptr;

                        // If the test expectations don't include a component object, then
                        // we auto-generate one. This is convenient for the many cases
                        // where the pattern has a default wildcard or empty string pattern
                        // for a component and the input is essentially empty.
                        static const picojson::value empty_expected_obj = json_parse("{ \"input\": \"\", \"groups\": {} }");
                        if (!expected_obj_ptr) {
                            expected_obj = empty_expected_obj;

                            // Next, we must treat default wildcards differently than empty string
                            // patterns.  The wildcard results in a capture group, but the empty
                            // string pattern does not.  The expectation object must list which
                            // components should be empty instead of wildcards in
                            // |exactly_empty_components|.
                            if (!entry_exactly_empty_components ||
                                !includes(entry_exactly_empty_components->get<picojson::array>(), component)) {
                                // expected_obj.groups['0'] = '';
                                expected_obj.get<picojson::object>().at("groups").get<picojson::object>()["0"] = picojson::value("");
                            }
                            //
                        }

                        std::string str{ "exec() result for " };
                        str.append(component);
                        assert_object_equals(tc,
                            expected_obj,
                            get_component_result(*exec_result, component),
                            str);
                    }
                });
            }
            catch (const upa::urlpattern_error& ex) {
                std::cout << "FAILURE: " << ex.what() << std::endl;
                return true;
            }
            catch (const std::exception& ex) {
                std::cout << "[ERR:invalid file]: " << ex.what() << std::endl;
                return false;
            }
        }
        return true;
    } };

    const int res = json_util::load_file(context, file_name);
    return res | ddt.result();
}

// create urlpattern

upa::urlpattern_init create_urlpattern_init(const picojson::object& obj) {
    upa::urlpattern_init init;
    for (const auto& [key, val] : obj)
        init.set(key, val.to_str());
    return init;
}

urlpattern create_urlpattern(const picojson::array& pattern_arr) {
    if (pattern_arr.empty())
        return urlpattern{};

    std::optional<std::string> arg_str;
    std::optional<std::string> arg_base;
    std::optional<upa::urlpattern_init> arg_init;
    upa::urlpattern_options arg_opt{};

    // 0 - input (string or URLPatternInit)
    if (pattern_arr[0].is<std::string>()) {
        arg_str = pattern_arr[0].get<std::string>();
    } else if (pattern_arr[0].is<picojson::object>()) {
        const auto obj = pattern_arr[0].get<picojson::object>();
        arg_init = create_urlpattern_init(obj);
    } else
        throw upa::urlpattern_error("The input must be a string or an object"); // failure

    // 1 - baseURL (string) or options (URLPatternOptions)
    int ind_options = -1;
    if (pattern_arr.size() >= 2) {
        if (pattern_arr[1].is<std::string>())
            arg_base = pattern_arr[1].get<std::string>();
        else if (pattern_arr.size() == 2)
            ind_options = 1;
        else
            throw upa::urlpattern_error("The baseURL must be a string"); // failure
    }

    // 2 - options (URLPatternOptions)
    if (pattern_arr.size() >= 3)
        ind_options = 2;

    if (ind_options >= 0) {
        if (pattern_arr[ind_options].is<picojson::object>()) {
            const auto obj = pattern_arr[ind_options].get<picojson::object>();
            if (obj.find("ignoreCase") != obj.end())
                arg_opt.ignore_case = obj.find("ignoreCase")->second.get<bool>();
        } else
            throw upa::urlpattern_error("The options must be an object"); // failure
    }

    if (arg_str)
        return arg_base
            ? urlpattern(*arg_str, *arg_base, arg_opt)
            : urlpattern(*arg_str, arg_opt);
    if (arg_base)
        throw upa::urlpattern_error("Unexpected base URL"); // failure
    if (arg_init)
        return urlpattern(*arg_init, arg_opt);

    return urlpattern(upa::urlpattern_init{}, arg_opt);
}

// Run urlpattern::test(...) and urlpattern::exec(...)

exec_args::exec_args(const picojson::array& input_arr) {
    if (input_arr.empty()) {
        arg_ = upa::urlpattern_init{};
    } else {
        if (input_arr[0].is<std::string>()) {
            arg_ = input_arr[0].get<std::string>();
        } else if (input_arr[0].is<picojson::object>()) {
            const auto obj = input_arr[0].get<picojson::object>();
            arg_ = create_urlpattern_init(obj);
        } // else // TODO: error in test file

        if (input_arr.size() >= 2) {
            if (input_arr[1].is<std::string>()) {
                base_ = input_arr[1].get<std::string>();
            } // else // TODO: error in test file
        }

        if (input_arr.size() >= 3) {
            // TODO: error in test file
        }
    }
}

bool urlpattern_test(const urlpattern& self, const exec_args& input) {
    if (std::holds_alternative<std::string>(input.arg_)) {
        return self.test(std::get<std::string>(input.arg_), input.base_);
    }
    if (std::holds_alternative<upa::urlpattern_init>(input.arg_)) {
        if (input.base_)
            throw upa::urlpattern_error("Unexpected base URL"); // failure
        return self.test(std::get<upa::urlpattern_init>(input.arg_));
    }
    // TODO: error in test file
    return false;
}

std::optional<upa::urlpattern_result> urlpattern_exec(const urlpattern& self, const exec_args& input) {
    if (std::holds_alternative<std::string>(input.arg_)) {
        return self.exec(std::get<std::string>(input.arg_), input.base_);
    }
    if (std::holds_alternative<upa::urlpattern_init>(input.arg_)) {
        if (input.base_)
            throw upa::urlpattern_error("Unexpected base URL"); // failure
        return self.exec(std::get<upa::urlpattern_init>(input.arg_));
    }
    // TODO: error in test file
    return std::nullopt;
}

// -----------------------------------------------------------------------------
// Test urlpattern_init

TEST_SUITE("urlpattern_init") {
    static constexpr std::pair<std::string_view, std::string_view> members[] = {
        { "protocol"sv, "protocol"sv },
        { "username"sv, "username"sv },
        { "password"sv, "password"sv },
        { "hostname"sv, "hostname"sv },
        { "port"sv, "port"sv },
        { "pathname"sv, "pathname"sv },
        { "search"sv, "search"sv },
        { "hash"sv, "hash"sv },
        { "baseURL"sv, "baseURL"sv }
    };
    static constexpr std::string_view not_members[] = {
        "no"sv,
        "no_such"sv,
        "no_such_member"sv,
        "post"sv
    };

    TEST_CASE("Test urlpattern_init set() & get()") {
        upa::urlpattern_init init;

        // get when no values are set
        for (auto [k, val] : members) {
            // structured bindings cannot be captured by lambda expressions until C++20
            auto key = k;
            CHECK_MESSAGE(init.get(key) == std::nullopt, "key = \"", key, "\"");
        }

        // set values of various string types
        init.set("protocol", "p-char-ptr");
        CHECK(init.get("protocol") == "p-char-ptr");
        init.set("protocol", "p-sv"sv);
        CHECK(init.get("protocol") == "p-sv");
        std::string str_lvalue{ "p-lvalue" };
        init.set("protocol", str_lvalue);
        CHECK(init.get("protocol") == "p-lvalue");
        init.set("protocol", std::string{ "p-rvalue" });
        CHECK(init.get("protocol") == "p-rvalue");

        // set values
        for (auto [key, val] : members)
            init.set(key, val);

        // check values
        for (auto [k, val] : members) {
            // structured bindings cannot be captured by lambda expressions until C++20
            auto key = k;
            CHECK_MESSAGE(init.get(key) == val, "key = \"", key, "\"");
        }

        // try to get the value of a non-existent member
        for (auto key : not_members) {
            CHECK_MESSAGE(init.get(key) == std::nullopt, "key = \"", key, "\"");
        }
    }
}

// -----------------------------------------------------------------------------
// Test is_identifier_start and is_identifier_part

TEST_SUITE("is_identifier_start & is_identifier_part") {
    TEST_CASE("Test is_identifier_start") {
        CHECK(upa::pattern::table::is_identifier_start('$'));
        CHECK(upa::pattern::table::is_identifier_start('_'));
        CHECK_FALSE(upa::pattern::table::is_identifier_start(0xE0100));
        CHECK_FALSE(upa::pattern::table::is_identifier_start(0x10FFFF));
    }
    TEST_CASE("Test is_identifier_part") {
        CHECK(upa::pattern::table::is_identifier_part('$'));
        CHECK(upa::pattern::table::is_identifier_part('_'));
        CHECK(upa::pattern::table::is_identifier_part(0xE0100));
        CHECK_FALSE(upa::pattern::table::is_identifier_part(0x10FFFF));
    }
}

// -----------------------------------------------------------------------------

int test_other(int argc, const char** argv) {
    std::cout << "========== Other tests ==========\n";
    return doctest::Context(argc, argv).run();
}

int main(int argc, const char** argv)
{
    int err = 0;

    err |= wpt_urlpattern_hasregexpgroups_tests();
    err |= wpt_urlpatterntests("wpt/urlpatterntestdata.json");
    err |= wpt_urlpatterntests("data/my-urlpatterntestdata.json");
    err |= test_other(argc, argv);

    return err;
}
