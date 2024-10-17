## String input

For string input, the library supports UTF-8, UTF-16, UTF-32 encodings and several string types, including `std::basic_string`, `std::basic_string_view`, null-terminated strings of any char type: `char`, `char8_t`, `char16_t`, `char32_t`, or `wchar_t`.

The ATL/MFC and Qt library string types are also supported. To use them you need to include the header files listed in the table below instead of `url.h`:

| Library | Supported string types | Include instead of `url.h` |
|-|-|-|
| ATL/MFC | `CSimpleStringT`, `CStringT`, `CFixedStringT` | `url_for_atl.h` |
| Qt | `QString`, `QStringView`, `QUtf8StringView` | `url_for_qt.h` |

Qt example:
```cpp
#include "upa/url_for_qt.h"
#include <QString>

int main() {
    QString str{"http://example.com/"};
    upa::url url{str};
    std::cout << url.href() << '\n';
    return 0;
}
```
