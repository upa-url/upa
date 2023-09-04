// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url.h"
#include "doctest-main.h"


TEST_CASE("Test setters with special URL's") {
    upa::url url;

    REQUIRE(upa::success(url.parse("ws://example.org/foo/bar", nullptr)));

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
    upa::url url;

    SUBCASE("non-special: protocol") {
        REQUIRE(upa::success(url.parse("non-special:/path", nullptr)));
        CHECK_EQ(url.href(), "non-special:/path");

        CHECK(url.hostname("example.net"));
        CHECK_EQ(url.href(), "non-special://example.net/path");

        CHECK(url.hostname(""));
        CHECK_EQ(url.href(), "non-special:///path");
    }

    SUBCASE("javascript: protocol") {
        REQUIRE(upa::success(url.parse("JavaScript:alert(1)", nullptr)));
        CHECK_EQ(url.href(), "javascript:alert(1)");

        CHECK(url.hash("#frag"));
        CHECK_EQ(url.href(), "javascript:alert(1)#frag");
    }
}

TEST_CASE("Test host setter with file URL") {
    upa::url url("file://h/p");

    SUBCASE("localhost") {
        CHECK(url.host("localhost"));
        CHECK(url.host() == "");
        CHECK(url.host_type() == upa::HostType::Empty);
    }
    SUBCASE("empty host") {
        CHECK(url.host(""));
        CHECK(url.host() == "");
        CHECK(url.host_type() == upa::HostType::Empty);
    }
}

TEST_CASE("Test setters (url_setter::start_part with use_strp_ = false)") {
    SUBCASE("Special URL") {
        upa::url url("http://h/p?query#frag");

        url.hash("");
        url.search("q");

        CHECK(url.href() == "http://h/p?q");
        CHECK(url.pathname() == "/p");
        CHECK(url.search() == "?q");
        CHECK(url.hash() == "");
    }
    SUBCASE("Non-special URL") {
        upa::url url("nonspec://host:123/path?query#frag");

        url.hash("");
        url.search("");
        url.pathname("");
        url.port("");
        url.hostname("h");

        CHECK(url.href() == "nonspec://h");
        CHECK(url.hostname() == "h");
        CHECK(url.port() == "");
        CHECK(url.pathname() == "");
        CHECK(url.search() == "");
        CHECK(url.hash() == "");

        url.search("q");

        CHECK(url.href() == "nonspec://h?q");
        CHECK(url.port() == "");
        CHECK(url.pathname() == "");
        CHECK(url.search() == "?q");
        CHECK(url.hash() == "");
    }
}

// Test alternative getters / setters

TEST_CASE("Test url::get_... getters") {
    upa::url url{ "ws://user:psw@h:54321\\p1\\p2?q#f" };

    CHECK(url.get_href() == "ws://user:psw@h:54321/p1/p2?q#f");
    CHECK(url.get_protocol() == "ws:");
    CHECK(url.get_username() == "user");
    CHECK(url.get_password() == "psw");
    CHECK(url.get_host() == "h:54321");
    CHECK(url.get_hostname() == "h");
    CHECK(url.get_port() == "54321");
    CHECK(url.get_path() == "/p1/p2?q");
    CHECK(url.get_pathname() == "/p1/p2");
    CHECK(url.get_search() == "?q");
    CHECK(url.get_hash() == "#f");
}

TEST_CASE("Test url::set_... setters") {
    upa::url url{ "ws://h" };

    CHECK(url.set_href("wss://host"));
    CHECK(url.get_href() == "wss://host/");

    CHECK(url.set_protocol("http"));
    CHECK(url.get_protocol() == "http:");

    CHECK(url.set_username("user"));
    CHECK(url.get_username() == "user");

    CHECK(url.set_password("psw"));
    CHECK(url.get_password() == "psw");

    CHECK(url.set_host("h:54321"));
    CHECK(url.get_host() == "h:54321");
    CHECK(url.get_hostname() == "h");
    CHECK(url.get_port() == "54321");

    CHECK(url.set_hostname("hostname"));
    CHECK(url.get_hostname() == "hostname");

    CHECK(url.set_port("61234"));
    CHECK(url.get_port() == "61234");

    CHECK(url.set_pathname("\\p1\\p2"));
    CHECK(url.get_pathname() == "/p1/p2");

    CHECK(url.set_search("q"));
    CHECK(url.get_search() == "?q");
    CHECK(url.get_path() == "/p1/p2?q");

    CHECK(url.set_hash("f"));
    CHECK(url.get_hash() == "#f");

    CHECK(url.href() == "http://user:psw@hostname:61234/p1/p2?q#f");
}
