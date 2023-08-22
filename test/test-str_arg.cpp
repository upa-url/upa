// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

//
// NOTE: To pass test this file must just compile without errors.
//

#include "str_arg.h"
//#include "doctest-main.h"


// Function to test

using namespace upa;

template <class ...Args, enable_if_str_arg_t<Args...> = 0>
inline std::size_t procfn(Args&&... args) {
    const auto inp = make_str_arg(std::forward<Args>(args)...);
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
    const size_t N = 3;
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

    procfn(arr, N);
    procfn(carr, N);
    procfn(ptr, N);
    procfn(cptr, N);
    procfn(vptr, N);

    // int size
    procfn(arr, int(N));
    procfn(cptr, int(N));

    procfn(arr, arr + N);
    procfn(carr, carr + N);
    procfn(ptr, ptr + N);
    procfn(cptr, cptr + N);
    procfn(vptr, vptr + N);

    const std::basic_string<CharT> str(arr);
    procfn(str);
    procfn(std::basic_string<CharT>(arr));

    upa::str_arg<CharT> arg(arr);
    procfn(arg);

    // test custom string
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
