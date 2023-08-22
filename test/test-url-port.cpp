// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url.h"
#include "doctest-main.h"


TEST_CASE("http scheme default port") {
    upa::url url;
    CHECK(upa::success(url.parse("http://aaa/", nullptr)));
    CHECK(url.port_int() == -1);
    CHECK(url.real_port_int() == 80);
}

TEST_CASE("http scheme 8080 port") {
    upa::url url;
    CHECK(upa::success(url.parse("http://aaa:8080/", nullptr)));
    CHECK(url.port_int() == 8080);
    CHECK(url.real_port_int() == 8080);
}

TEST_CASE("non-special scheme default port") {
    upa::url url;
    CHECK(upa::success(url.parse("non-special://aaa/", nullptr)));
    CHECK(url.port_int() == -1);
    CHECK(url.real_port_int() == -1);
}

TEST_CASE("non-special scheme 123 port") {
    upa::url url;
    CHECK(upa::success(url.parse("non-special://aaa:123/", nullptr)));
    CHECK(url.port_int() == 123);
    CHECK(url.real_port_int() == 123);
}

TEST_CASE("port overflow") {
    upa::url url;

    // https://github.com/whatwg/url/issues/257#issuecomment-285553590
    CHECK(upa::success(url.parse("http://example.net:65535", nullptr)));
    CHECK(url.port_int() == 65535);
    CHECK_FALSE(upa::success(url.parse("http://example.net:65536", nullptr)));

    CHECK(upa::success(url.parse("asdf://host:65535", nullptr)));
    CHECK(url.port_int() == 65535);
    CHECK_FALSE(upa::success(url.parse("asdf://host:65536", nullptr)));
}
