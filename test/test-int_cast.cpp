#include "int_cast.h"
#include "doctest-main.h"


TEST_CASE("whatwg::checked_diff") {
    SUBCASE("char -> int") {
        const char max_char = std::numeric_limits<char>::max();
        const char min_char = std::numeric_limits<char>::min();
        CHECK_EQ(whatwg::checked_diff<int>(max_char, min_char), int(max_char) - int(min_char));
        CHECK_EQ(whatwg::checked_diff<int>(min_char, max_char), int(min_char) - int(max_char));
    }

    const int max_int = std::numeric_limits<int>::max();
    const int min_int = std::numeric_limits<int>::min();

    SUBCASE("int -> int") {
        // min_int
        CHECK_EQ(whatwg::checked_diff<int>(min_int + 1, 1), min_int);
        CHECK_EQ(whatwg::checked_diff<int>(min_int, -1), min_int + 1);
        CHECK_EQ(whatwg::checked_diff<int>(min_int, 0), min_int);
        CHECK_THROWS(whatwg::checked_diff<int>(min_int, 1)); // min_int - 1
        // max_int
        CHECK_EQ(whatwg::checked_diff<int>(max_int, 0), max_int);
        CHECK_THROWS(whatwg::checked_diff<int>(max_int, -1)); // max_int + 1
        // min_int, max_int
        CHECK_THROWS(whatwg::checked_diff<int>(max_int, min_int)); // > max_int
        CHECK_THROWS(whatwg::checked_diff<int>(min_int, max_int)); // < min_int
    }

    SUBCASE("int -> unsigned") {
        CHECK_EQ(whatwg::checked_diff<unsigned>(max_int, min_int), uint64_t(int64_t(max_int) - int64_t(min_int)));
        // negative result
        CHECK_THROWS(whatwg::checked_diff<unsigned>(min_int, max_int));
        CHECK_THROWS(whatwg::checked_diff<unsigned>(0, 1));
    }

    SUBCASE("int64_t -> int") {
        const int64_t max_int64 = std::numeric_limits<int64_t>::max();
        const int64_t min_int64 = std::numeric_limits<int64_t>::min();

        CHECK_EQ(whatwg::checked_diff<int>(max_int64, max_int64 - max_int), max_int);
        CHECK_THROWS(whatwg::checked_diff<int>(max_int64, max_int64 - max_int - 1)); // > max_int

        CHECK_EQ(whatwg::checked_diff<int>(max_int64 + min_int, max_int64), min_int);
        CHECK_THROWS(whatwg::checked_diff<int>(max_int64 + min_int - 1, max_int64)); // < min_int

        CHECK_EQ(whatwg::checked_diff<int>(min_int64, min_int64 - min_int), min_int);
        CHECK_THROWS(whatwg::checked_diff<int>(min_int64, min_int64 - min_int + 1)); // < min_int

        CHECK_EQ(whatwg::checked_diff<int>(min_int64 + max_int, min_int64), max_int);
        CHECK_THROWS(whatwg::checked_diff<int>(min_int64 + max_int + 1, min_int64)); // > max_int
    }
}
