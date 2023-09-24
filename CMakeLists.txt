# FindICU requires 3.7
# add_link_options() requires 3.13
cmake_minimum_required(VERSION 3.13)

set(CMAKE_SUPPRESS_REGENERATION true)

# use ICU_ROOT
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# Project settings
project(upa_url LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ standard")

# Options
option(URL_BUILD_TESTS "Build the URL tests." ON)
option(URL_BUILD_FUZZER "Build the URL fuzzer." OFF)
option(URL_BUILD_EXAMPLES "Build the URL examples." OFF)
option(URL_BUILD_TOOLS "Build tools." OFF)
# library options
option(URL_AMALGAMATED "Use amalgamated URL library source." OFF)
# tests build options
option(URL_TEST_COVERAGE "Build tests with code coverage reporting" OFF)
option(URL_TEST_COVERAGE_CLANG "Build tests with Clang source-based code coverage" OFF)
option(URL_TEST_SANITIZER "Build tests with Clang sanitizer" OFF)
option(URL_TEST_VALGRIND "Run tests with Valgrind" OFF)

# AFL, Honggfuzz, or Clang libFuzzer
if (URL_BUILD_FUZZER)
  set(URL_BUILD_TESTS OFF)
  get_filename_component(cxx_name ${CMAKE_CXX_COMPILER} NAME)
  # https://aflplus.plus/docs/env_variables/
  string(REGEX MATCH "^afl-((c|clang|clang-fast|clang-lto|g)\\+\\+|g\\+\\+-fast)$" AFL ${cxx_name})
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    string(REGEX MATCH "^hfuzz-clang\\+\\+$" HFUZZ ${cxx_name})
    if (NOT HFUZZ)
      # Clang libFuzzer or AFL++
      add_compile_options(-O1 -g -fsanitize=fuzzer)
      add_link_options(-g -fsanitize=fuzzer)
    endif()
  else()
    set(AFL_MAIN ${AFL})
  endif()
endif()

# Code coverage reporting
if (URL_TEST_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # add flags for GCC & LLVM/Clang
  add_compile_options(-O0 -g --coverage)
  add_link_options(--coverage)
endif()

# Clang source-based code coverage
# https://clang.llvm.org/docs/SourceBasedCodeCoverage.html
# https://llvm.org/docs/CommandGuide/llvm-cov.html#show-command
if (URL_TEST_COVERAGE_CLANG)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fprofile-instr-generate=${CMAKE_CURRENT_BINARY_DIR}/%m.profraw -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate)
  endif()
endif()

# Clang sanitizer
if (URL_TEST_SANITIZER)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-O1 -g -fsanitize=address -fsanitize=undefined -fsanitize=integer -fno-omit-frame-pointer)
    add_link_options(-g -fsanitize=address -fsanitize=undefined)
  endif()
endif()

# Valgrind
if (URL_TEST_VALGRIND)
  find_program(MEMORYCHECK valgrind)
  set(MEMORYCHECK_COMMAND "${MEMORYCHECK} --error-exitcode=1 --leak-check=full")
  separate_arguments(MEMORYCHECK_COMMAND)
endif()

# are URL and ICU libraries needed
if (URL_BUILD_TESTS OR URL_BUILD_FUZZER OR URL_BUILD_EXAMPLES)
  set(URL_USE_LIBS ON)
endif()

include_directories(deps)

if (URL_USE_LIBS)
  find_package(ICU REQUIRED COMPONENTS i18n uc)

  if (URL_AMALGAMATED)
    add_library(upaurl STATIC
      single_include/upa/url.cpp)
    target_include_directories(upaurl
      INTERFACE single_include)
  else()
    add_library(upaurl STATIC
      src/url.cpp
      src/url_idna.cpp
      src/url_ip.cpp
      src/url_percent_encode.cpp
      src/url_search_params.cpp
      src/url_utf.cpp)
    target_include_directories(upaurl
      PUBLIC include)
  endif()
  add_library(upa::url ALIAS upaurl)
  target_include_directories(upaurl PRIVATE ${ICU_INCLUDE_DIR})
  target_link_libraries(upaurl INTERFACE ICU::i18n ICU::uc)
endif()

# Test targets

if (URL_BUILD_TESTS)
  enable_testing()

  if (URL_AMALGAMATED)
    set(test_files
      test/test-amalgamate.cpp
    )
  else()
    set(test_files
      test/test-buffer.cpp
      test/test-ipv4.cpp
      test/test-ipv6.cpp
      test/test-str_arg.cpp
      test/test-utf.cpp
      test/test-util.cpp
      test/test-url.cpp
      test/test-url-port.cpp
      test/test-url-setters.cpp
      test/test-url_host.cpp
      test/test-url_percent_encode.cpp
      test/test-url_search_params.cpp
      test/wpt-url.cpp
      test/wpt-url-setters-stripping.cpp
      test/wpt-url_search_params.cpp
      test/wpt-urlencoded-parser.cpp
    )
  endif()
  foreach(file ${test_files})
    get_filename_component(test_name ${file} NAME_WE)

    add_executable(${test_name} ${file})
    target_link_libraries(${test_name} upaurl)

    add_test(NAME ${test_name}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${test_name})

    if (URL_TEST_VALGRIND)
      add_test(NAME ${test_name}_valgrind
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
        COMMAND ${MEMORYCHECK_COMMAND} ${CMAKE_CURRENT_BINARY_DIR}/${test_name})
    endif()
  endforeach()
endif()

# Fuzzer targets

if (URL_BUILD_FUZZER)
  set(fuzz_files
    test/fuzz-url.cpp
  )
  foreach(file ${fuzz_files})
    get_filename_component(fuzz_name ${file} NAME_WE)

    if (AFL_MAIN)
      add_executable(${fuzz_name} ${file} test/afl-main.cpp)
    else()
      add_executable(${fuzz_name} ${file})
    endif()
    target_link_libraries(${fuzz_name} upaurl)
  endforeach()
endif()

# Example's targets

if (URL_BUILD_EXAMPLES)
  add_executable(urlparse examples/urlparse.cpp)
  target_link_libraries(urlparse upaurl)
endif()

# Tool's targets

if (URL_BUILD_TOOLS)
  add_executable(dumpCharBitSets tools/dumpCharBitSets.cpp)
  set_property(TARGET dumpCharBitSets PROPERTY CXX_STANDARD 17)
  target_include_directories(dumpCharBitSets PRIVATE include)
endif()

# Install

include(GNUInstallDirs)

install(
  DIRECTORY include/upa
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  TARGETS upaurl
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
