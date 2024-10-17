# Upa URL

Upa URL is [WHATWG URL Standard](https://url.spec.whatwg.org/) compliant <b>U</b>RL <b>pa</b>rser library written in C++.

The library is self-contained with no dependencies and requires a compiler that supports C++11 or later. It is known to compile with Clang 4, GCC 4.9, Microsoft Visual Studio 2015 or later. Can also be compiled to WebAssembly, see demo: https://upa-url.github.io/demo/

## Features and standard conformance

This library is up to date with the URL Standard published on
[19 August 2024 (commit 5861e02)](https://url.spec.whatwg.org/commit-snapshots/5861e023bbd695c92190aa6bb7cb3c6717dab33b/) and supports internationalized domain names as specified in the [UTS46 Unicode IDNA Compatibility Processing version 15.1.0](https://www.unicode.org/reports/tr46/tr46-31.html).

It implements:
1. [URL class](https://url.spec.whatwg.org/#url-class): `upa::url`
2. [URLSearchParams class](https://url.spec.whatwg.org/#interface-urlsearchparams): `upa::url_search_params`
3. [URL record](https://url.spec.whatwg.org/#concept-url): `upa::url` has functions to examine URL record members: `get_part_view(PartType t)`, `is_empty(PartType t)` and `is_null(PartType t)`
4. [URL equivalence](https://url.spec.whatwg.org/#url-equivalence): `upa::equals` function
5. [Percent decoding and encoding](https://url.spec.whatwg.org/#percent-encoded-bytes) functions: `upa::percent_decode`, `upa::percent_encode` and `upa::encode_url_component`

It has some differences from the standard:
1. Setters of the `upa::url` class are implemented as functions, which return `true` if value is accepted.
2. The `href` setter does not throw on parsing failure, but returns `false`.

Upa URL contains features not specified in the standard:
1. The `upa::url` class has `path` getter (to get `pathname` concatenated with `search`)
2. Function to convert file system path to file URL: `upa::url_from_file_path`
3. Function to get file system path from file URL: `upa::path_from_file_url`
4. Experimental URLHost class (see proposal: https://github.com/whatwg/url/pull/288): `upa::url_host`
5. The `upa::url_search_params` class has a few additional functions: `remove`, `remove_if`

For string input, the library supports UTF-8, UTF-16, UTF-32 encodings and several string types, including `std::basic_string`, `std::basic_string_view`, null-terminated strings of any char type: `char`, `char8_t`, `char16_t`, `char32_t`, or `wchar_t`. See ["String input"](doc/string_input.md) for more information.

## Installation

The simplest way is to use two amalgamated files: `url.h` and `url.cpp`. You can download them from [releases page](https://github.com/upa-url/upa/releases), or if you have installed Python, then generate them by running `tools/amalgamate.sh` script (`tools/amalgamate.bat` on Windows). The files will be created in the `single_include/upa` directory.

### CMake

The library can be built and installed using CMake 3.13 or later. To build and install to default directory (usually `/usr/local` on Linux) run following commands:
```sh
cmake -B build -DUPA_BUILD_TESTS=OFF
cmake --build build
cmake --install build
```

To use library add `find_package(upa REQUIRED)` and link to `upa::url` target in your CMake project:
```cmake
find_package(upa REQUIRED)
...
target_link_libraries(exe-target PRIVATE upa::url)
```

#### Embedding

The entire library source tree can be placed in subdirectory (say `url/`) of your project and then included in it with `add_subdirectory()`:
```cmake
add_subdirectory(url)
...
target_link_libraries(exe-target PRIVATE upa::url)
```

#### Embedding with FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(upa
  GIT_REPOSITORY https://github.com/upa-url/upa.git
  GIT_SHALLOW TRUE
  GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(upa)
...
target_link_libraries(exe-target PRIVATE upa::url)
```

#### Embedding with CPM.cmake script

If you are using the [CPM.cmake script](https://github.com/cpm-cmake/CPM.cmake) and have included it in your `CMakeLists.txt`, then:

```cmake
CPMAddPackage("gh:upa-url/upa@1.0.0")
...
target_link_libraries(exe-target PRIVATE upa::url)
```

### vcpkg

For use in C++17 or later projects, install Upa URL with `vcpkg install upa-url`.

For use in C++11 and C++14 projects: `vcpkg install upa-url[cxx11]`.

## Usage

In source files, that use this library, the `upa/url.h` must be  included:
```cpp
#include "upa/url.h"
```

If you are using CMake, see the [CMake section](#cmake) for how to link to the library. Alternatively, if you are using amalgamated files, then add the amalgamated `url.cpp` file to your project, otherwise add all the files from the `src/` directory to your project.

### Examples

Parse input string using `url::parse` function and output URL components:
```cpp
#include "upa/url.h"
#include <iostream>
#include <string>

int main() {
    upa::url url;
    std::string input;

    std::cout << "Enter URL to parse, or empty line to exit\n";
    while (std::getline(std::cin, input) && !input.empty()) {
        if (upa::success(url.parse(input))) {
            std::cout << "     href: " << url.href() << '\n';
            std::cout << "   origin: " << url.origin() << '\n';
            std::cout << " protocol: " << url.protocol() << '\n';
            std::cout << " username: " << url.username() << '\n';
            std::cout << " password: " << url.password() << '\n';
            std::cout << " hostname: " << url.hostname() << '\n';
            std::cout << "     port: " << url.port() << '\n';
            std::cout << " pathname: " << url.pathname() << '\n';
            std::cout << "   search: " << url.search() << '\n';
            std::cout << "     hash: " << url.hash() << '\n';
        } else {
            std::cout << " URL parse error\n";
        }
    }
}
```

Parse URL against base URL using `url` constructor:
```cpp
try {
    upa::url url{ "/new path?query", "https://example.org/path" };
    std::cout << url.href() << '\n';
}
catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << '\n';
}
```

Use setters of the `url` class:
```cpp
upa::url url;
if (upa::success(url.parse("http://host/"))) {
    url.protocol("https:");
    url.host("example.com:443");
    url.pathname("kelias");
    url.search("z=7");
    url.hash("top");
    std::cout << url.href() << '\n';
}
```

Enumerate search parameters of URL:
```cpp
upa::url url{ "wss://h?first=last&op=hop&a=b" };
for (const auto& param : url.search_params()) {
    std::cout << param.first << " = " << param.second << '\n';
}
```

Remove search parameters whose names start with "utm_" (requires C++20):
```cpp
upa::url url{ "https://example.com/?id=1&utm_source=twitter.com&utm_medium=social" };
url.search_params().remove_if([](const auto& param) {
    return param.first.starts_with("utm_");
});
std::cout << url.href() << '\n'; // https://example.com/?id=1
```

Convert filesystem path to file URL:
```cpp
try {
    auto url = upa::url_from_file_path("/home/opa/file.txt", upa::file_path_format::posix);
    std::cout << url.href() << '\n'; // file:///home/opa/file.txt
}
catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << '\n';
}
```

## License

This library is licensed under the [BSD 2-Clause License](https://opensource.org/license/bsd-2-clause/). It contains portions of modified source code from the Chromium project, licensed under the [BSD 3-Clause License](https://opensource.org/license/bsd-3-clause/), and the ICU project, licensed under the [UNICODE LICENSE V3](https://www.unicode.org/license.txt).

See the `LICENSE` file for more information.
