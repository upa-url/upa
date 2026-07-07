## C++20 modules support

> [!WARNING]
> Support for C++20 modules is considered experimental.

### Requirements

To compile the library as a C++20 module, you need CMake 3.28.2 or newer, Ninja 1.11 or newer, and one of the following compilers:
* GCC 15 or newer
* LLVM/Clang 16.0 or newer (to build static library)
* LLVM/Clang 19.1 or newer (to build static or shared library)
* MSVC toolset 14.50 (Visual Studio 2026) or newer

### CMake

To compile as a C++20 module, specify `UPA_MODULE=ON`. For example:
```sh
cmake -B build -G Ninja -DUPA_MODULE=ON -DUPA_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Then install:
```sh
cmake --install build
```

### Usage

To use library add `find_package(upa REQUIRED)` and link to `upa::url-module` target in your `CMakeLists.txt`:
```cmake
find_package(upa REQUIRED)
...
target_link_libraries(exe-target PRIVATE upa::url-module)
```

In the C++ file you need to import `upa.url`. If you need to include other headers, they must be above the `import` directive. For example:
```cpp
#include <iostream>

import upa.url;

int main() {
    upa::url url;

    if (upa::success(url.parse("https://example.com/p/a?q=Q#f")))
        std::cout << url.get_href() << '\n';
    return 0;
}
```

References:
* [cmake-cxxmodules(7)](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html) - the latest status of C++20 modules support in CMake.
* [C++20 Modules, CMake, And Shared Libraries](https://crascit.com/2024/04/04/cxx-modules-cmake-shared-libraries/)
