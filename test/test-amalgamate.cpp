// Copyright 2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url.h"
#include "upa/public_suffix_list.h"

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

TEST_CASE("public_suffix_list push_line & get_suffix") {
    upa::public_suffix_list psl;
    // push
    upa::public_suffix_list::push_context ctx;
    psl.push_line(ctx, "github.io");
    // get info
    CHECK(psl.get_suffix("upa-url.github.io") == "github.io");
}
