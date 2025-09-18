# Upa URL

Upa URL is [WHATWG URL Standard](https://url.spec.whatwg.org/) compliant <b>U</b>RL <b>pa</b>rser library written in C++.

The library is self-contained with no dependencies and requires a compiler that supports C++17 or later (for previous major version that requires C++11 or later, see [the v1.x branch](https://github.com/upa-url/upa/tree/v1.x)).
It is known to compile with Clang 7, GCC 8, Microsoft Visual Studio 2017 or later. Can also be compiled to WebAssembly, see demo: https://upa-url.github.io/demo/

## Features and standard conformance

This library is up to date with the URL Standard published on
[14 March 2025 (commit 6c78200)](https://url.spec.whatwg.org/commit-snapshots/6c782003a2d53b1feecd072d1006eb8f1d65fb2d/) and supports internationalized domain names as specified in the [UTS46 Unicode IDNA Compatibility Processing version 17.0.0](https://www.unicode.org/reports/tr46/tr46-35.html).

It implements:
1. [URL class](https://url.spec.whatwg.org/#url-class): `upa::url`
2. [URLSearchParams class](https://url.spec.whatwg.org/#interface-urlsearchparams): `upa::url_search_params`
3. [URL record](https://url.spec.whatwg.org/#concept-url): `upa::url` has functions to examine URL record members: `get_part_view(PartType t)`, `is_empty(PartType t)` and `is_null(PartType t)`
4. [URL equivalence](https://url.spec.whatwg.org/#url-equivalence): `upa::equals` function
5. [Percent decoding and encoding](https://url.spec.whatwg.org/#percent-encoded-bytes) functions: `upa::percent_decode`, `upa::percent_encode` and `upa::encode_url_component`
6. [Public Suffix List](https://publicsuffix.org/) class `upa::public_suffix_list` to get [public suffix](https://url.spec.whatwg.org/#host-public-suffix) and [registrable domain](https://url.spec.whatwg.org/#host-registrable-domain) of a host.

It has some differences from the standard:
1. Setters of the `upa::url` class are implemented as functions, which return `true` if value is accepted.
2. The `href` setter does not throw on parsing failure, but returns `false`.

Upa URL contains features not specified in the standard:
1. The `upa::url` class has `path` getter (to get `pathname` concatenated with `search`)
2. The `upa::url` class has the `get_part_pos` function to get the start and end position of the specified URL component in a URL string.
3. Function to convert file system path to file URL: `upa::url_from_file_path`
4. Functions to get file system path from file URL: `upa::path_from_file_url` and `upa::fs_path_from_file_url`
5. Experimental URLHost class (see proposal: https://github.com/whatwg/url/pull/288): `upa::url_host`
6. The `upa::url_search_params` class has a few additional functions: `remove`, `remove_if`

For string input, the library supports UTF-8, UTF-16, UTF-32 encodings and several string types, including `std::basic_string`, `std::basic_string_view`, null-terminated strings of any char type: `char`, `char8_t`, `char16_t`, `char32_t`, or `wchar_t`. See ["String input"](doc/string_input.md) for more information.

## Installation

The simplest way is to use amalgamated files: `url.h` and `url.cpp`. For the Public Suffix List functionality, you will also need `public_suffix_list.h` and `public_suffix_list.cpp`. You can download them from [releases page](https://github.com/upa-url/upa/releases), or if you have installed Python, then generate them by running `tools/amalgamate.sh` script (`tools\amalgamate.bat` on Windows). The files will be created in the `single_include/upa` directory.

### CMake

The library can be built and installed using CMake 3.13 or later. To build and install to default directory (usually `/usr/local` on Linux) run following commands:
```sh
cmake -B build -DUPA_BUILD_TESTS=OFF
cmake --build build
cmake --install build
```

This installs the Upa URL as a static library, which is recommended. To build and install it as a shared library, add the `-DBUILD_SHARED_LIBS=ON` option to the first command.

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
  GIT_TAG v2.3.0
)
FetchContent_MakeAvailable(upa)
...
target_link_libraries(exe-target PRIVATE upa::url)
```

#### Embedding with CPM.cmake script

If you are using the [CPM.cmake script](https://github.com/cpm-cmake/CPM.cmake) and have included it in your `CMakeLists.txt`, then:

```cmake
CPMAddPackage("gh:upa-url/upa@2.3.0")
...
target_link_libraries(exe-target PRIVATE upa::url)
```

### Conan

Install the latest version of Upa URL:
```sh
conan install --requires=upa-url/[*]
```

### Homebrew

```sh
brew install upa-url/tap/upa-url
```

### vcpkg

```sh
vcpkg install upa-url
```

## Usage

Source files using this library must include `upa/url.h`:
```cpp
#include "upa/url.h"
```

To use the Public Suffix List, you must also include `upa/public_suffix_list.h`:
```cpp
#include "upa/public_suffix_list.h"
```

If you are using CMake, see the [CMake section](#cmake) for how to link to the library. Alternatively, if you are using amalgamated files, then add the amalgamated `url.cpp` file (and optionaly amalgamated `public_suffix_list.cpp`) to your project, otherwise add all the files from the `src/` directory to your project.

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

Load the [Public Suffix List](https://publicsuffix.org/) and get [public suffix](https://url.spec.whatwg.org/#host-public-suffix) of a hostname:
```cpp
#include "upa/public_suffix_list.h"
#include <iostream>

int main() {
    upa::public_suffix_list psl;
    if (psl.load("public_suffix_list.dat"))
        std::cout << psl.get_suffix("upa-url.github.io.",
            upa::public_suffix_list::option::allow_trailing_dot) << '\n'; // github.io.
}
```

## Using as a shared library

For information on building as a shared library, see the [CMake section](#cmake).

For performance reasons, it is recommended that you define the `UPA_LIB_IMPORT` macro when building your program against the shared library. If you installed the library with CMake and are building a program with CMake, this macro will already be defined.

Use the `upa::check_version()` function to check if a shared library is compatible with a compiled program. It returns `true` if they are ABI compatible.

## Building and running tests

Clone the repository (`git clone https://github.com/upa-url/upa.git`) and run `init.sh` (`init.bat` on Windows) in the repository's root directory to download the dependencies and test data files. These files are necessary for building and running tests, but not for the library itself.

To build and run the tests, execute the following commands:
```sh
cmake -B build
cmake --build build
ctest --output-on-failure --test-dir build
```

On Windows, you need to specify the build configuration, for example, `Debug`:
```sh
cmake -B build
cmake --build build --config Debug
ctest --output-on-failure --test-dir build -C Debug
```

## License

This library is licensed under the [BSD 2-Clause License](https://opensource.org/license/bsd-2-clause/). It contains portions of modified source code from the Chromium project, licensed under the [BSD 3-Clause License](https://opensource.org/license/bsd-3-clause/), and the ICU project, licensed under the [UNICODE LICENSE V3](https://www.unicode.org/license.txt).

See the `LICENSE` file for more information.
