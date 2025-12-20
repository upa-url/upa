// Copyright 2023-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url.h"
#include "upa/urlpattern.h"
#include "upa/public_suffix_list.h"
#include "upa/regex_engine_std.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"


TEST_CASE("url constructor & getters") {
    upa::url url{ "seg1/seg2?a=b#frag", "https://user:psw@example.com:321/seg0/file" };

    CHECK(url.href() == "https://user:psw@example.com:321/seg0/seg1/seg2?a=b#frag");
    CHECK(url.origin() == "https://example.com:321");
    CHECK(url.protocol() == "https:");
    CHECK(url.username() == "user");
    CHECK(url.password() == "psw");
    CHECK(url.host() == "example.com:321");
    CHECK(url.hostname() == "example.com");
    CHECK(url.port() == "321");
    CHECK(url.path() == "/seg0/seg1/seg2?a=b");
    CHECK(url.pathname() == "/seg0/seg1/seg2");
    CHECK(url.search() == "?a=b");
    CHECK(url.hash() == "#frag");
}

TEST_CASE("urlpattern") {
    using urlpattern = upa::urlpattern<upa::regex_engine_std>;
    using namespace std::string_view_literals;

    upa::urlpattern_init init;
    init.set("protocol", "https");
    init.set("hostname", "*.:subdomain.lt");
    CHECK(init.get("protocol") == "https");
    CHECK(init.get("hostname") == "*.:subdomain.lt");

    urlpattern up{ init };
    CHECK(up.get_protocol() == "https");
    CHECK(up.get_hostname() == "*.:subdomain.lt");
    CHECK(up.test("https://www.lrt.lt/mediateka"));
    auto res = up.exec("https://www.lrt.lt/mediateka");
    REQUIRE(res);
    CHECK(res->protocol.input == "https");
    CHECK(res->protocol.groups.size() == 0);
    CHECK(res->hostname.input == "www.lrt.lt");
    CHECK(res->hostname.groups.size() == 2);
    CHECK(res->hostname.groups["0"] == "www");
    CHECK(res->hostname.groups["subdomain"] == "lrt");
    CHECK(res->pathname.input == "/mediateka");
    CHECK(res->pathname.groups.size() == 1);
    CHECK(res->pathname.groups["0"] == "/mediateka");
}

TEST_CASE("public_suffix_list push_line & get_suffix") {
    upa::public_suffix_list psl;
    // push
    upa::public_suffix_list::push_context ctx;
    psl.push_line(ctx, "github.io");
    // get info
    CHECK(psl.get_suffix("upa-url.github.io") == "github.io");
}
