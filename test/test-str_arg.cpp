//
// NOTE: To pass test this file must just compile without errors.
//
#include "str_arg.h"
//#include "doctest-main.h"


using namespace whatwg;

template <class CharT, enable_if_char_t<CharT, int> = 0>
inline std::size_t procfn(const CharT* first, const CharT* last) {
    return last - first;
}

template <class ...Args, enable_if_str_arg_t<Args...> = 0>
inline std::size_t procfn(const Args&... args) {
    return procfn(str_arg::begin(args...), str_arg::end(args...));
}


template <class CharT>
inline void test_char() {
    const size_t N = 3;
    CharT arr[] = { '1', '2', '3', 0 };
    const CharT carr[] = { '1', '2', '3', 0 };
    CharT* ptr = arr;
    const CharT* cptr = arr;

    procfn(arr);
    procfn(carr);
    procfn(ptr);
    procfn(cptr);

    procfn(arr, N);
    procfn(carr, N);
    procfn(ptr, N);
    procfn(cptr, N);

    procfn(ptr, ptr + N);
    procfn(cptr, cptr + N);

    procfn(std::basic_string<CharT>(arr));
}

int main() {
    test_char<char>();
    test_char<wchar_t>();
    test_char<char16_t>();
    test_char<char32_t>();

#if 1
    const char* volatile ptr = "VL";
    //procfn(ptr);
    //procfn(ptr, 2);
    procfn(ptr, ptr + 2);
#endif

    return 0;
}
