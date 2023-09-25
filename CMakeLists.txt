# FindICU requires 3.7
# add_link_options() requires 3.13
cmake_minimum_required(VERSION 3.13)

set(CMAKE_SUPPRESS_REGENERATION true)

# use ICU_ROOT
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# Project settings
project(upa_url VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ standard")

# Options
option(URL_BUILD_TESTS "Build the URL tests." ON)
option(URL_BUILD_FUZZER "Build the URL fuzzer." OFF)
option(URL_BUILD_EXAMPLES "Build the URL examples." OFF)
option(URL_BUILD_TOOLS "Build tools." OFF)
option(URL_INSTALL "Generate the install target." ON)
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

include_directories(deps)

# Are Upa URL and ICU libraries needed?
if (URL_BUILD_TESTS OR URL_BUILD_FUZZER OR URL_BUILD_EXAMPLES OR URL_INSTALL OR NOT URL_BUILD_TOOLS)
  # Upa URL library name
  set(upa_url_lib upa_url)

  find_package(ICU REQUIRED COMPONENTS i18n uc)

  if (URL_AMALGAMATED)
    add_library(${upa_url_lib} STATIC
      single_include/upa/url.cpp)
    target_include_directories(${upa_url_lib}
      INTERFACE single_include)
  else()
    add_library(${upa_url_lib} STATIC
      src/url.cpp
      src/url_idna.cpp
      src/url_ip.cpp
      src/url_percent_encode.cpp
      src/url_search_params.cpp
      src/url_utf.cpp)
    target_include_directories(${upa_url_lib} PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>)
  endif()
  add_library(upa::url ALIAS ${upa_url_lib})
  target_include_directories(${upa_url_lib} PRIVATE ${ICU_INCLUDE_DIR})
  target_link_libraries(${upa_url_lib} INTERFACE ICU::i18n ICU::uc)
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
    target_link_libraries(${test_name} ${upa_url_lib})

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
    target_link_libraries(${fuzz_name} ${upa_url_lib})
  endforeach()
endif()

# Example's targets

if (URL_BUILD_EXAMPLES)
  add_executable(urlparse examples/urlparse.cpp)
  target_link_libraries(urlparse ${upa_url_lib})
endif()

# Tool's targets

if (URL_BUILD_TOOLS)
  add_executable(dumpCharBitSets tools/dumpCharBitSets.cpp)
  set_property(TARGET dumpCharBitSets PROPERTY CXX_STANDARD 17)
  target_include_directories(dumpCharBitSets PRIVATE include)
endif()

# Install

if (URL_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(
    DIRECTORY include/upa
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(
    TARGETS ${upa_url_lib}
    EXPORT ${upa_url_lib}-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  install(
    EXPORT ${upa_url_lib}-targets
    FILE ${upa_url_lib}-targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${upa_url_lib}
  )

  # generate the config file that includes the exports
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${upa_url_lib}-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${upa_url_lib}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${upa_url_lib}
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )
  # generate the version file for the config file
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${upa_url_lib}-config-version.cmake
    COMPATIBILITY SameMinorVersion
  )

  # install the generated configuration files
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${upa_url_lib}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${upa_url_lib}-config-version.cmake
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${upa_url_lib}"
  )

  # generate the export targets for the build tree
  # needs to be after the install(TARGETS) command
  export(
    EXPORT ${upa_url_lib}-targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${upa_url_lib}-targets.cmake
  )
endif()
