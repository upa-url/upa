#include "url.h"
#include "doctest-main.h"


TEST_CASE("http scheme default port") {
    whatwg::url url;
    CHECK(whatwg::success(url.parse("http://aaa/", nullptr)));
    CHECK(url.port_int() == -1);
    CHECK(url.real_port_int() == 80);
}

TEST_CASE("http scheme 8080 port") {
    whatwg::url url;
    CHECK(whatwg::success(url.parse("http://aaa:8080/", nullptr)));
    CHECK(url.port_int() == 8080);
    CHECK(url.real_port_int() == 8080);
}

TEST_CASE("non-special scheme default port") {
    whatwg::url url;
    CHECK(whatwg::success(url.parse("non-special://aaa/", nullptr)));
    CHECK(url.port_int() == -1);
    CHECK(url.real_port_int() == -1);
}

TEST_CASE("non-special scheme 123 port") {
    whatwg::url url;
    CHECK(whatwg::success(url.parse("non-special://aaa:123/", nullptr)));
    CHECK(url.port_int() == 123);
    CHECK(url.real_port_int() == 123);
}
