// Copyright 2016-2021 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include <string>
#include "doctest-main.h"
#include "buffer.h" // must be included after doctest.h

using str = std::char_traits<char>;

TEST_CASE("whatwg::simple_buffer<char, 16>") {
    whatwg::simple_buffer<char, 16> buff;

    buff.reserve(10);
    buff.resize(3);
    str::copy(buff.data(), "ABC", 3);
    buff.push_back('D');

    CHECK(buff.capacity() >= 10);
    CHECK(buff.size() == 4);
    CHECK(str::compare(buff.data(), "ABCD", 4) == 0);

    // data() const
    const auto& cbuff = buff;
    CHECK(cbuff.data() != nullptr);
    CHECK(str::compare(cbuff.data(), "ABCD", 4) == 0);

    // test append
    std::string aaa("123456789");
    std::string bbb("abcdefgh-");

    buff.append(aaa.data(), aaa.data() + aaa.length());
    CHECK(buff.capacity() >= 13);
    CHECK(buff.size() == 13);
    CHECK(str::compare(buff.data(), "ABCD123456789", 13) == 0);

    buff.append(bbb.data(), bbb.data() + bbb.length());
    CHECK(buff.capacity() >= 22);
    CHECK(buff.size() == 22);
    CHECK(str::compare(buff.data(), "ABCD123456789abcdefgh-", 22) == 0);

    buff.append(bbb.data(), bbb.data() + bbb.length());
    CHECK(buff.capacity() >= 31);
    CHECK(buff.size() == 31);
    CHECK(str::compare(buff.data(), "ABCD123456789abcdefgh-abcdefgh-", 31) == 0);

    buff.push_back('\0');
    CHECK(buff.capacity() >= 32);
    CHECK(buff.size() == 32);
    CHECK(buff.data()[31] == 0);

    buff.pop_back();
    CHECK(buff.size() == 31);

    buff.clear();
    CHECK(buff.empty());
    CHECK(buff.size() == 0);
}

TEST_CASE("whatwg::simple_buffer<char, 4>") {
    whatwg::simple_buffer<char, 4> buff;

    CHECK(buff.empty());

    // make buff.size() == buff.capacity()
    std::string aaa("1234");
    buff.append(aaa.data(), aaa.data() + aaa.length());

    CHECK_FALSE(buff.empty());
    CHECK(buff.capacity() == 4);
    CHECK(buff.size() == 4);
    CHECK(str::compare(buff.data(), "1234", 4) == 0);

    INFO("buff.push_back grows capacity");
    buff.push_back('5');
    CHECK(buff.capacity() >= 5);
    CHECK(buff.size() == 5);
    CHECK(str::compare(buff.data(), "12345", 5) == 0);
}

TEST_CASE("whatwg::simple_buffer<char, 2> constructor with initial capacity 4") {
    whatwg::simple_buffer<char, 2> buff(4);

    std::string aaa("1234");
    buff.append(aaa.data(), aaa.data() + aaa.length());

    CHECK(buff.capacity() == 4);
    CHECK(buff.size() == 4);
    CHECK(str::compare(buff.data(), "1234", 4) == 0);

    INFO("grow capacity when initial buffer is allocated dynamically");
    buff.reserve(8);
    std::string bbb("5678");
    buff.append(bbb.data(), bbb.data() + bbb.length());

    CHECK(buff.capacity() == 8);
    CHECK(buff.size() == 8);
    CHECK(str::compare(buff.data(), "12345678", 4) == 0);
}

TEST_CASE("whatwg::simple_buffer<char, 2> constructor with initial capacity 2") {
    whatwg::simple_buffer<char, 2> buff(2);

    std::string aaa("1234567890123456");
    buff.append(aaa.data(), aaa.data() + aaa.length());

    CHECK(buff.capacity() >= 16);
    CHECK(buff.size() == 16);
    CHECK(str::compare(buff.data(), "1234567890123456", 16) == 0);
}

TEST_CASE("whatwg::simple_buffer<char, 0>") {
    whatwg::simple_buffer<char, 0> buff;

    buff.push_back('A');
    CHECK(buff.capacity() >= 1);
    CHECK(buff.size() == 1);
    CHECK(buff.data() != nullptr);
    CHECK(buff.data()[0] == 'A');
}

TEST_CASE("whatwg::simple_buffer internal test") {
    whatwg::simple_buffer<char16_t> buff;
    buff.internal_test();
}
