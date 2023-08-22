// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "util.h"
#include "doctest-main.h"


TEST_CASE("upa::util::checked_diff") {
    SUBCASE("char -> int") {
        const char max_char = std::numeric_limits<char>::max();
        const char min_char = std::numeric_limits<char>::min();
        CHECK_EQ(upa::util::checked_diff<int>(max_char, min_char), int(max_char) - int(min_char));
        CHECK_EQ(upa::util::checked_diff<int>(min_char, max_char), int(min_char) - int(max_char));
    }

    const int max_int = std::numeric_limits<int>::max();
    const int min_int = std::numeric_limits<int>::min();

    SUBCASE("int -> int") {
        // min_int
        CHECK_EQ(upa::util::checked_diff<int>(min_int + 1, 1), min_int);
        CHECK_EQ(upa::util::checked_diff<int>(min_int, -1), min_int + 1);
        CHECK_EQ(upa::util::checked_diff<int>(min_int, 0), min_int);
        CHECK_THROWS(upa::util::checked_diff<int>(min_int, 1)); // min_int - 1
        // max_int
        CHECK_EQ(upa::util::checked_diff<int>(max_int, 0), max_int);
        CHECK_THROWS(upa::util::checked_diff<int>(max_int, -1)); // max_int + 1
        // min_int, max_int
        CHECK_THROWS(upa::util::checked_diff<int>(max_int, min_int)); // > max_int
        CHECK_THROWS(upa::util::checked_diff<int>(min_int, max_int)); // < min_int
    }

    SUBCASE("int -> unsigned") {
        CHECK_EQ(upa::util::checked_diff<unsigned>(max_int, min_int), uint64_t(int64_t(max_int) - int64_t(min_int)));
        // negative result
        CHECK_THROWS(upa::util::checked_diff<unsigned>(min_int, max_int));
        CHECK_THROWS(upa::util::checked_diff<unsigned>(0, 1));
    }

    SUBCASE("int64_t -> int") {
        const int64_t max_int64 = std::numeric_limits<int64_t>::max();
        const int64_t min_int64 = std::numeric_limits<int64_t>::min();

        CHECK_EQ(upa::util::checked_diff<int>(max_int64, max_int64 - max_int), max_int);
        CHECK_THROWS(upa::util::checked_diff<int>(max_int64, max_int64 - max_int - 1)); // > max_int

        CHECK_EQ(upa::util::checked_diff<int>(max_int64 + min_int, max_int64), min_int);
        CHECK_THROWS(upa::util::checked_diff<int>(max_int64 + min_int - 1, max_int64)); // < min_int

        CHECK_EQ(upa::util::checked_diff<int>(min_int64, min_int64 - min_int), min_int);
        CHECK_THROWS(upa::util::checked_diff<int>(min_int64, min_int64 - min_int + 1)); // < min_int

        CHECK_EQ(upa::util::checked_diff<int>(min_int64 + max_int, min_int64), max_int);
        CHECK_THROWS(upa::util::checked_diff<int>(min_int64 + max_int + 1, min_int64)); // > max_int
    }
}

TEST_CASE("upa::util::add_sizes") {
    constexpr auto max_size = std::numeric_limits<std::size_t>::max();

    SUBCASE("success") {
        CHECK(upa::util::add_sizes(max_size - 1, 1, max_size) == max_size);
        CHECK(upa::util::add_sizes(1, max_size - 1, max_size) == max_size);
    }

    SUBCASE("failure") {
        CHECK_THROWS(upa::util::add_sizes(2, max_size - 1, max_size));
        CHECK_THROWS(upa::util::add_sizes(max_size - 1, 2, max_size));
        CHECK_THROWS(upa::util::add_sizes(max_size, max_size, max_size));
    }
}

// upa::util::has_xn_label

template <typename CharT>
static bool has_xn_label(const CharT* domain) {
    std::basic_string<CharT> str(domain);
    return upa::util::has_xn_label(str.data(), str.data() + str.length());
};

TEST_CASE("upa::util::has_xn_label") {
    // true
    CHECK(has_xn_label("xn--"));
    CHECK(has_xn_label("abc.XN--"));
    CHECK(has_xn_label("abc.Xn--a"));
    CHECK(has_xn_label("xN--a.cba"));
    CHECK(has_xn_label(L"xn--"));
    CHECK(has_xn_label(L"abc.xn--"));
    CHECK(has_xn_label(L"abc.Xn--a"));
    CHECK(has_xn_label(L"xN--a.cba"));
    // false
    CHECK_FALSE(has_xn_label("xn-"));
    CHECK_FALSE(has_xn_label("abc.xn-"));
    CHECK_FALSE(has_xn_label("an--"));
    CHECK_FALSE(has_xn_label("abc.xz--"));
    CHECK_FALSE(has_xn_label(L"xn-"));
    CHECK_FALSE(has_xn_label(L"abc.xn-"));
    CHECK_FALSE(has_xn_label(L"an--"));
    CHECK_FALSE(has_xn_label(L"abc.xz--"));
}
