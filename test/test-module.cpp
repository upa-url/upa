// Copyright 2026 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#include "doctest-main.h"

#include "upa/regex_engine_srell.h"

#include <unordered_set>

import upa.url;

// Custom string class and it's specialization

template <typename CharT>
class CustomString {
public:
    CustomString(const CharT* data, std::size_t length)
        : data_(data), length_(length) {}
    const CharT* GetData() const { return data_; }
    std::size_t GetLength() const { return length_; }
private:
    const CharT* data_ = nullptr;
    std::size_t length_ = 0;
};

namespace upa {

template<typename CharT>
struct str_arg_char<CustomString<CharT>> {
    using type = CharT;

    static str_arg<CharT> to_str_arg(const CustomString<CharT>& str) {
        return { str.GetData(), str.GetLength() };
    }
};

} // namespace upa

TEST_CASE("upa::url") {
    upa::url url("https://user:psw@example.com:1234/p/a?q=Q#f");

    CHECK(url.protocol() == "https:");
    CHECK(url.username() == "user");
    CHECK(url.password() == "psw");
    CHECK(url.host() == "example.com:1234");
    CHECK(url.hostname() == "example.com");
    CHECK(url.port_int() == 1234);
    CHECK(url.pathname() == "/p/a");
    CHECK(url.search() == "?q=Q");
    CHECK(url.hash() == "#f");

    CHECK(url.pathname("../path"));
    CHECK(url.pathname() == "/path");

    upa::url url2{ url };
    CHECK(url2.href() == url.href());

    CHECK(upa::url::can_parse("about:blank"));

    // Custom string
    CHECK(upa::success(url.parse(CustomString<char>("about:blank", 11))));
    CHECK(url.href() == "about:blank");
}

TEST_CASE("std::unordered_set<upa::url>") {
    std::unordered_set<upa::url> aibe;
    aibe.emplace(upa::url{ "about:blank" });
    aibe.emplace(upa::url{ "file:///path" });
    aibe.emplace(upa::url{ "https://example.org/" });

    CHECK_FALSE(aibe.contains(upa::url{ "about:about" }));
    CHECK(aibe.contains(upa::url{ "about:blank" }));
}

TEST_CASE("upa::url_search_params") {
    constexpr auto get_eq =
        [](upa::url_search_params& usp, const char* name, const char* value) {
            return usp.get(name) && *usp.get(name) == value;
        };

    upa::url_search_params usp{ "a=2&c=1" };

    CHECK(usp.size() == 2);
    CHECK(get_eq(usp, "a", "2"));
    CHECK(get_eq(usp, "c", "1"));
    CHECK_FALSE(usp.has("b"));
    usp.append("b", "3");
    CHECK(usp.size() == 3);
    CHECK(get_eq(usp, "b", "3"));
    usp.set("a", "4");
    CHECK(get_eq(usp, "a", "4"));
    usp.remove("c");
    CHECK_FALSE(usp.has("c"));
}

TEST_CASE("upa::url_host") {
    upa::url_host host("www.example.com");
    CHECK(host.to_string() == "www.example.com");
    CHECK(host.type() == upa::HostType::Domain);

    upa::url_host host2{ host };
    CHECK(host2.to_string() == "www.example.com");
}

TEST_CASE("upa::public_suffix_list") {
    upa::public_suffix_list psl;
    upa::public_suffix_list::push_context ctx;
    psl.push_line(ctx, "co.uk");
    psl.finalize(ctx);

    const auto registrable_domain = upa::public_suffix_list::option::registrable_domain;
    CHECK(psl.get_suffix_view("www.example.co.uk", registrable_domain) == "example.co.uk");

    upa::public_suffix_list psl2{ std::move(psl) };
    CHECK(psl2.get_suffix_view("www.miestas.co.uk", registrable_domain) == "miestas.co.uk");

    // Invalid hostname
    CHECK(psl2.get_suffix_view("a^b.co.uk") == "");
}

TEST_CASE("upa::urlpattern") {
    upa::urlpattern<upa::regex_engine_srell> urlp{ "https://*.example.com/*" };

    CHECK(urlp.test("https://www.example.com/path"));
    CHECK_FALSE(urlp.test("https://example.com/#1"));
}

TEST_CASE("upa::urlpattern_init") {
    upa::urlpattern_init urlp_init;

    urlp_init.protocol = upa::make_string(std::string("http:"));
    urlp_init.hostname = upa::make_string("*.lt");
    urlp_init.port = upa::make_string(L"80");
    urlp_init.pathname = upa::make_string(u"/*");
    urlp_init.set("search", upa::make_string(U"q=*"));
    urlp_init.set("hash", "h");

    upa::urlpattern<upa::regex_engine_srell> urlp{ urlp_init };
    CHECK(urlp.test("http://www.example.lt/path?q=1#h"));
    CHECK_FALSE(urlp.test("http://www.example.lt/path"));
}
