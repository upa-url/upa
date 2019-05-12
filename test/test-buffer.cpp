#include <string>
#include "buffer.h"
#include "doctest-main.h"

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
}
