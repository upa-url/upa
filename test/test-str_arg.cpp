// Copyright 2016-2026 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#include "upa/str_arg.h"
#include "doctest-main.h"
#include <array>
#include <vector>

// Function to test

template <class StrT, upa::enable_if_str_arg_t<StrT> = 0>
inline std::size_t procfn(const StrT& str) {
    const auto inp = upa::make_str_arg(str);
    return std::distance(inp.begin(), inp.end());
}

// Custom string class convertible to std::basic_string_view

template <typename CharT>
class ConvertibleString {
public:
    ConvertibleString(const CharT* data, std::size_t length)
        : data_(data), length_(length) {}
    operator std::basic_string_view<CharT>() const noexcept {
        return { data_, length_ };
    }
private:
    const CharT* data_ = nullptr;
    std::size_t length_ = 0;
};

// Custom string class and it's specialization

template <typename CharT>
class CustomString {
public:
    CustomString(const CharT* data, std::size_t length)
        : data_(data), length_(length) {}
    const CharT* GetData() const { return data_; }
    std::size_t GetLength() const { return length_; }
private:
    const CharT* data_ = nullptr;
    std::size_t length_ = 0;
};

namespace upa {

template<typename CharT>
struct str_arg_char<CustomString<CharT>> {
    using type = CharT;

    static str_arg<CharT> to_str_arg(const CustomString<CharT>& str) {
        return { str.GetData(), str.GetLength() };
    }
};

} // namespace upa


// Test char type

TEST_CASE_TEMPLATE_DEFINE("test_char", CharT, test_char) {
    const std::size_t N = 3;
    CharT arr[] = { '1', '2', '3', 0 };
    const CharT carr[] = { '1', '2', '3', 0 };
    CharT* ptr = arr;
    const CharT* cptr = arr;
    const CharT* volatile vptr = arr;

    CHECK(procfn(arr) == N);
    CHECK(procfn(carr) == N);
    CHECK(procfn(ptr) == N);
    CHECK(procfn(cptr) == N);
    CHECK(procfn(vptr) == N);

    // upa::str_arg
    upa::str_arg<CharT> arg{ arr, N };
    CHECK(procfn(arg) == N);
    const upa::str_arg carg{ arr, N };
    CHECK(procfn(carg) == N);

    CHECK(procfn(upa::str_arg{ arr, N }) == N);
    CHECK(procfn(upa::str_arg{ carr, N }) == N);
    CHECK(procfn(upa::str_arg{ ptr, N }) == N);
    CHECK(procfn(upa::str_arg{ cptr, N }) == N);
    CHECK(procfn(upa::str_arg{ vptr, N }) == N);
    // int size
    CHECK(procfn(upa::str_arg{ arr, static_cast<int>(N) }) == N);
    CHECK(procfn(upa::str_arg{ cptr, static_cast<int>(N) }) == N);

    CHECK(procfn(upa::str_arg{ arr, arr + N }) == N);
    CHECK(procfn(upa::str_arg{ carr, carr + N }) == N);
    CHECK(procfn(upa::str_arg{ ptr, ptr + N }) == N);
    CHECK(procfn(upa::str_arg{ cptr, cptr + N }) == N);
    CHECK(procfn(upa::str_arg{ vptr, vptr + N }) == N);

    // has data() and size() members
    CHECK(procfn(std::array<CharT, N>{ '1', '2', '3'}) == N);
    CHECK(procfn(std::vector<CharT>{ '1', '2', '3'}) == N);

    // std::basic_string
    const std::basic_string<CharT> str{ arr };
    CHECK(procfn(str) == N);
    CHECK(procfn(std::basic_string<CharT>{ arr }) == N);

    // std::basic_string_view
    const std::basic_string_view<CharT> strv{ arr };
    CHECK(procfn(strv) == N);
    CHECK(procfn(std::basic_string_view<CharT>{ arr }) == N);

    // custom strings
    CHECK(procfn(ConvertibleString<CharT>{cptr, N}) == N);
    CHECK(procfn(CustomString<CharT>{cptr, N}) == N);
}

TEST_CASE_TEMPLATE_INVOKE(test_char, char, wchar_t, char16_t, char32_t);
#ifdef __cpp_char8_t
TEST_CASE_TEMPLATE_INVOKE(test_char, char8_t);
#endif
