// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

//
// NOTE: To pass test this file must just compile without errors.
//

#include "upa/str_arg.h"
//#include "doctest-main.h"


// Function to test

template <class StrT, upa::enable_if_str_arg_t<StrT> = 0>
inline std::size_t procfn(StrT&& str) {
    const auto inp = upa::make_str_arg(std::forward<StrT>(str));
    return std::distance(inp.begin(), inp.end());
}


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

template <class CharT>
inline void test_char() {
    const std::size_t N = 3;
    CharT arr[] = { '1', '2', '3', 0 };
    const CharT carr[] = { '1', '2', '3', 0 };
    CharT* ptr = arr;
    const CharT* cptr = arr;
    const CharT* volatile vptr = arr;

    procfn(arr);
    procfn(carr);
    procfn(ptr);
    procfn(cptr);
    procfn(vptr);

    // upa::str_arg
    upa::str_arg<CharT> arg(arr);
    procfn(arg);
    const upa::str_arg<CharT> carg(arr);
    procfn(carg);

    procfn(upa::str_arg<CharT>{arr, N});
    procfn(upa::str_arg<CharT>{carr, N});
    procfn(upa::str_arg<CharT>{ptr, N});
    procfn(upa::str_arg<CharT>{cptr, N});
    procfn(upa::str_arg<CharT>{vptr, N});
    // int size
    procfn(upa::str_arg<CharT>(arr, static_cast<int>(N)));
    procfn(upa::str_arg<CharT>(cptr, static_cast<int>(N)));

    procfn(upa::str_arg<CharT>{arr, arr + N});
    procfn(upa::str_arg<CharT>{carr, carr + N});
    procfn(upa::str_arg<CharT>{ptr, ptr + N});
    procfn(upa::str_arg<CharT>{cptr, cptr + N});
    procfn(upa::str_arg<CharT>{vptr, vptr + N});

    // std::basic_string
    const std::basic_string<CharT> str(arr);
    procfn(str);
    procfn(std::basic_string<CharT>(arr));

    // custom string
    procfn(CustomString<CharT>{cptr, N});
}

int main() {
    test_char<char>();
    test_char<wchar_t>();
    test_char<char16_t>();
    test_char<char32_t>();
#ifdef __cpp_char8_t
    test_char<char8_t>();
#endif

    return 0;
}
