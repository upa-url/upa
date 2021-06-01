// Copyright 2016-2021 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url.h"
#include "doctest-main.h"


TEST_CASE("Test setters with special URL's") {
    whatwg::url url;

    REQUIRE(whatwg::success(url.parse("ws://example.org/foo/bar", nullptr)));

    SUBCASE("Check getters") {
        CHECK_EQ(url.href(), "ws://example.org/foo/bar");
        CHECK_EQ(url.protocol(), "ws:");
        CHECK_EQ(url.host(), "example.org");
        CHECK_EQ(url.pathname(), "/foo/bar");
    }

    SUBCASE("url::href(...)") {
        CHECK_FALSE(url.href("wss://%00/foo/bar"));
        CHECK_EQ(url.href(), "ws://example.org/foo/bar");

        CHECK(url.href("wss://host/foo/bar"));
        CHECK_EQ(url.href(), "wss://host/foo/bar");
    }

    SUBCASE("switch to http: protocol") {
        // if setter sets value, then returns true
        CHECK(url.protocol("http:"));
        CHECK_EQ(url.protocol(), "http:");

        CHECK(url.username("user01"));
        CHECK_EQ(url.username(), "user01");

        CHECK(url.password("pass@01"));
        CHECK_EQ(url.password(), "pass%4001");

        CHECK(url.host("example.org:81"));
        CHECK_EQ(url.host(), "example.org:81");

        CHECK(url.hostname("example.net"));
        CHECK_EQ(url.host(), "example.net:81");
        CHECK_EQ(url.hostname(), "example.net");

        CHECK(url.port("88"));
        CHECK_EQ(url.host(), "example.net:88");
        CHECK_EQ(url.port_int(), 88);

        CHECK(url.port(""));
        CHECK_EQ(url.host(), "example.net");

        CHECK(url.pathname("/path"));
        CHECK_EQ(url.pathname(), "/path");

        CHECK(url.hash("#frag"));
        CHECK_EQ(url.hash(), "#frag");

        CHECK(url.search("?a=3"));
        CHECK_EQ(url.search(), "?a=3");

        // test path
        CHECK(url.pathname("/other/path"));
        CHECK_EQ(url.pathname(), "/other/path");
    }

    SUBCASE("switch to file: protocol") {
        CHECK(url.protocol("file:"));
        CHECK_EQ(url.protocol(), "file:");

        CHECK(url.hostname("localhost"));
        CHECK_EQ(url.hostname(), "");

        CHECK(url.hostname("example.org"));
        CHECK_EQ(url.hostname(), "example.org");

        // windows drive letters
        CHECK(url.pathname("/c|/../path"));
        CHECK_EQ(url.pathname(), "/c:/path");
    }
}

TEST_CASE("Test setters with non-special URL's") {
    whatwg::url url;

    SUBCASE("non-special: protocol") {
        REQUIRE(whatwg::success(url.parse("non-special:/path", nullptr)));
        CHECK_EQ(url.href(), "non-special:/path");

        CHECK(url.hostname("example.net"));
        CHECK_EQ(url.href(), "non-special://example.net/path");

        CHECK(url.hostname(""));
        CHECK_EQ(url.href(), "non-special:///path");
    }

    SUBCASE("javascript: protocol") {
        REQUIRE(whatwg::success(url.parse("JavaScript:alert(1)", nullptr)));
        CHECK_EQ(url.href(), "javascript:alert(1)");

        CHECK(url.hash("#frag"));
        CHECK_EQ(url.href(), "javascript:alert(1)#frag");
    }
}

TEST_CASE("Test host setter with file URL") {
    whatwg::url url("file://h/p");

    SUBCASE("localhost") {
        CHECK(url.host("localhost"));
        CHECK(url.host() == "");
        CHECK(url.host_type() == whatwg::HostType::Empty);
    }
    SUBCASE("empty host") {
        CHECK(url.host(""));
        CHECK(url.host() == "");
        CHECK(url.host_type() == whatwg::HostType::Empty);
    }
}
