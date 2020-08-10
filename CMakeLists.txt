# FindICU requires 3.7
cmake_minimum_required(VERSION 3.7)

set(CMAKE_SUPPRESS_REGENERATION true)

# use ICU_ROOT
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# Project settings
project(tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ standard")

# Options
option(URL_BUILD_TESTS "Build the URL tests." ON)
option(URL_BUILD_FUZZER "Build the URL fuzzer." OFF)
option(URL_BUILD_EXAMPLES "Build the URL examples." OFF)
option(URL_BUILD_TOOLS "Build tools." OFF)
# tests build options
option(URL_TEST_COROUTINE "Build coroutine tests (MSVC)" OFF)
option(URL_TEST_SANITIZER "Build tests with Clang sanitizer" OFF)

# AFL or Clang libFuzzer
if (URL_BUILD_FUZZER)
  set(URL_BUILD_TESTS OFF)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    string(REGEX MATCH "(afl-clang-fast|afl-clang)\\+\\+$" AFL ${CMAKE_CXX_COMPILER})
    if (NOT AFL)
      # Clang libFuzzer
      add_compile_options(-O1 -g -fsanitize=fuzzer)
      add_link_options(-g -fsanitize=fuzzer)
    endif()
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    string(REGEX MATCH "afl-g\\+\\+$" AFL ${CMAKE_CXX_COMPILER})
  endif()
endif()

# coroutine support
if (URL_TEST_COROUTINE)
  if (MSVC)
    add_compile_options(/await)
  endif()
endif()

# Clang sanitizer
if (URL_TEST_SANITIZER)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-O1 -g -fsanitize=address -fsanitize=undefined -fsanitize=integer -fno-omit-frame-pointer)
    add_link_options(-g -fsanitize=address -fsanitize=undefined)
  endif()
endif()

# are URL and ICU libraries needed
if (URL_BUILD_TESTS OR URL_BUILD_FUZZER OR URL_BUILD_EXAMPLES)
  set(URL_USE_LIBS ON)
endif()

include_directories(
  deps
  src)

if (URL_USE_LIBS)
  find_package(ICU REQUIRED COMPONENTS i18n uc)
  find_package(Threads)

  add_library(cppurl STATIC
    src/url.cpp
    src/url_idna.cpp
    src/url_ip.cpp
    src/url_percent_encode.cpp
    src/url_search_params.cpp
    src/url_utf.cpp)
  target_include_directories(cppurl PRIVATE ${ICU_INCLUDE_DIR})
endif()

# Test targets

if (URL_BUILD_TESTS)
  enable_testing()

  set(test_files
    test/test-buffer.cpp
    test/test-int_cast.cpp
    test/test-ipv4.cpp
    test/test-ipv6.cpp
    test/test-str_arg.cpp
    test/test-utf.cpp
    test/test-url.cpp
    test/test-url-port.cpp
    test/test-url-setters.cpp
    test/test-url_percent_encode.cpp
    test/test-url_search_params.cpp
    test/wpt-url.cpp
    test/wpt-url-setters-stripping.cpp
    test/wpt-url_search_params.cpp
    test/wpt-urlencoded-parser.cpp
  )
  foreach(file ${test_files})
    get_filename_component(test_name ${file} NAME_WE)

    add_executable(${test_name} ${file})
    target_link_libraries(${test_name} cppurl ICU::i18n ICU::uc Threads::Threads)

    add_test(NAME ${test_name}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${test_name})
  endforeach()
endif()

# Fuzzer targets

if (URL_BUILD_FUZZER)
  set(fuzz_files
    test/fuzz-url.cpp
  )
  foreach(file ${fuzz_files})
    get_filename_component(fuzz_name ${file} NAME_WE)

    if (AFL)
      add_executable(${fuzz_name} ${file} test/afl-main.cpp)
    else()
      add_executable(${fuzz_name} ${file})
    endif()
    target_link_libraries(${fuzz_name} cppurl ICU::i18n ICU::uc Threads::Threads)
  endforeach()
endif()

# Example's targets

if (URL_BUILD_EXAMPLES)
  add_executable(urlparse examples/urlparse.cpp)
  target_link_libraries(urlparse cppurl ICU::i18n ICU::uc Threads::Threads)
endif()

# Tool's targets

if (URL_BUILD_TOOLS)
  add_executable(dumpCharBitSets tools/dumpCharBitSets.cpp)
  set_property(TARGET dumpCharBitSets PROPERTY CXX_STANDARD 17)
endif()
