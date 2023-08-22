// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url_utf.h"
#include "str_arg.h"
#include "doctest-main.h"


using namespace upa;

template <typename T>
static uint32_t first_codepoint(T&& strUtf) {
    const auto inp = make_str_arg(strUtf);
    const auto* first = inp.begin();
    const auto* last = inp.end();
    return url_utf::read_utf_char(first, last).value;
}

TEST_CASE("url_utf::read_utf_char with UTF-8") {
    CHECK(first_codepoint(u8"\u007F") == 0x7F);
    // U+0080..U+07FF
    CHECK(first_codepoint(u8"\u0080") == 0x0080);
    CHECK(first_codepoint(u8"\u07FF") == 0x07FF);
    // U+0800..U+FFFF except surrogates
    CHECK(first_codepoint(u8"\u0800") == 0x0800);
    CHECK(first_codepoint(u8"\uD7FF") == 0xD7FF);
    CHECK(first_codepoint(u8"\uE000") == 0xE000);
    CHECK(first_codepoint(u8"\uFFFF") == 0xFFFF);
    // U+10000..U+10FFFF
    CHECK(first_codepoint(u8"\U00010000") == 0x10000);
    CHECK(first_codepoint(u8"\U0010FFFF") == 0x10FFFF);
}

TEST_CASE("url_utf::read_utf_char with invalid UTF-8") {
    // must return U+FFFD - REPLACEMENT CHARACTER
    CHECK(first_codepoint(std::string{ char(0xC2), char('x') }) == 0xFFFD);
    CHECK(first_codepoint(std::string{ char(0xF0), char(0x90), char('x') }) == 0xFFFD);
}
