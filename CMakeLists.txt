# add_link_options() requires 3.13
cmake_minimum_required(VERSION 3.13)

set(CMAKE_SUPPRESS_REGENERATION true)

# https://cmake.org/cmake/help/latest/policy/CMP0074.html
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# Extract version from url_version.h
file(READ include/upa/url_version.h upa_url_version_h)
if (NOT upa_url_version_h MATCHES "[ \t]+UPA_URL_VERSION[ \t]+\"([0-9]+(\.[0-9]+)+)\"")
  message(FATAL_ERROR "Cannot get UPA_URL_VERSION from url_version.h")
endif()
set(UPA_URL_VERSION ${CMAKE_MATCH_1})
message(STATUS "Upa URL version: ${UPA_URL_VERSION}")
# Create SOVERSION from major and minor version numbers
string(REGEX MATCH "^[0-9]+\.[0-9]+" UPA_URL_SOVERSION ${UPA_URL_VERSION})
message(STATUS "Upa URL SOVERSION: ${UPA_URL_SOVERSION}")

# Project settings
project(upa_url VERSION ${UPA_URL_VERSION} LANGUAGES CXX)
# The ${upa_lib_name} is used to create the config file name:
# ${upa_lib_name}-config.cmake
# It also must be used as the package name argument to find_package
set(upa_lib_name upa)
# Exported name for library target files; also used to create an alias
# target: upa::${upa_export_name}
set(upa_export_name url)
# The ${upa_lib_target} is the logical library target name; also used
# to create the library filename
set(upa_lib_target upa_url)

if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Upa URL is top level project?
if (NOT DEFINED UPA_MAIN_PROJECT)
  set(UPA_MAIN_PROJECT OFF)
  if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(UPA_MAIN_PROJECT ON)
  endif()
endif()

# Options
option(URL_BUILD_TESTS "Build the Upa URL tests." ${UPA_MAIN_PROJECT})
option(UPA_BUILD_TESTS "Build the Upa URL tests." ${URL_BUILD_TESTS})
option(UPA_BUILD_BENCH "Build the Upa URL benchmarks." OFF)
option(UPA_BUILD_FUZZER "Build the Upa URL fuzzer." OFF)
option(UPA_BUILD_EXAMPLES "Build the Upa URL examples." OFF)
option(UPA_BUILD_EXTRACTED "Build Upa URL examples extracted from the docs." OFF)
option(UPA_INSTALL "Generate the install target." ON)
# library options
option(UPA_AMALGAMATED "Use amalgamated URL library source." OFF)
# tests build options
option(UPA_TEST_URL_FOR_QT "Build tests with Qt strings" OFF)
option(UPA_TEST_COVERAGE "Build tests with code coverage reporting" OFF)
option(UPA_TEST_COVERAGE_CLANG "Build tests with Clang source-based code coverage" OFF)
option(UPA_TEST_SANITIZER "Build tests with Clang sanitizer" OFF)
option(UPA_TEST_VALGRIND "Run tests with Valgrind" OFF)

# AFL, Honggfuzz, or Clang libFuzzer
if (UPA_BUILD_FUZZER)
  set(UPA_BUILD_TESTS OFF)
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

# Tests with Qt strings
if (UPA_TEST_URL_FOR_QT)
  find_package(Qt6 REQUIRED COMPONENTS Core)
endif()

# Code coverage reporting
if (UPA_TEST_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # add flags for GCC & LLVM/Clang
  add_compile_options(-O0 -g --coverage)
  add_link_options(--coverage)
endif()

# Clang source-based code coverage
# https://clang.llvm.org/docs/SourceBasedCodeCoverage.html
# https://llvm.org/docs/CommandGuide/llvm-cov.html#show-command
if (UPA_TEST_COVERAGE_CLANG)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fprofile-instr-generate=${CMAKE_CURRENT_BINARY_DIR}/%m.profraw -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate)
  endif()
endif()

# Clang sanitizer
if (UPA_TEST_SANITIZER)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-O1 -g -fsanitize=address -fsanitize=undefined -fsanitize=integer -fno-omit-frame-pointer)
    add_link_options(-g -fsanitize=address -fsanitize=undefined)
  endif()
endif()

# Valgrind
if (UPA_TEST_VALGRIND)
  find_program(MEMORYCHECK valgrind)
  set(MEMORYCHECK_COMMAND "${MEMORYCHECK} --error-exitcode=1 --leak-check=full")
  separate_arguments(MEMORYCHECK_COMMAND)
endif()

include_directories(deps)

# Is the Upa URL library needed?
if (UPA_BUILD_TESTS OR UPA_BUILD_BENCH OR UPA_BUILD_FUZZER OR UPA_BUILD_EXAMPLES OR
    UPA_BUILD_EXTRACTED OR UPA_INSTALL)
  if (UPA_AMALGAMATED)
    add_library(${upa_lib_target}
      single_include/upa/url.cpp
      single_include/upa/public_suffix_list.cpp)
    target_include_directories(${upa_lib_target}
      INTERFACE single_include)
  else()
    add_library(${upa_lib_target}
      src/idna.cpp
      src/public_suffix_list.cpp
      src/url.cpp
      src/url_ip.cpp
      src/url_percent_encode.cpp
      src/url_search_params.cpp
      src/url_utf.cpp)
    target_include_directories(${upa_lib_target} PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>)
  endif()
  if (BUILD_SHARED_LIBS)
    target_compile_definitions(${upa_lib_target} PRIVATE UPA_LIB_EXPORT
      INTERFACE UPA_LIB_IMPORT)
  endif ()
  # Alias target for the library
  add_library(upa::${upa_export_name} ALIAS ${upa_lib_target})
  set_target_properties(${upa_lib_target} PROPERTIES
    EXPORT_NAME ${upa_export_name}
    VERSION ${UPA_URL_VERSION}
    SOVERSION ${UPA_URL_SOVERSION}
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
  # GCC 8 requires linking with stdc++fs to use std::filesystem
  target_link_libraries(${upa_lib_target} PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)
endif()

# Test targets

if (UPA_BUILD_TESTS)
  enable_testing()

  if (UPA_AMALGAMATED)
    set(test_files
      test/test-amalgamate.cpp
    )
  else()
    set(test_files
      test/test-buffer.cpp
      test/test-ipv4.cpp
      test/test-ipv6.cpp
      test/test-public_suffix_list.cpp
      test/test-str_arg.cpp
      test/test-utf.cpp
      test/test-util.cpp
      test/test-url.cpp
      test/test-url-port.cpp
      test/test-url-setters.cpp
      test/test-url_for_.cpp
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
    target_link_libraries(${test_name} PRIVATE ${upa_lib_target})

    if ("${test_name}" STREQUAL "test-url_for_")
      if (UPA_TEST_URL_FOR_QT)
        target_compile_definitions(${test_name} PRIVATE UPA_TEST_URL_FOR_QT)
        target_link_libraries(${test_name} PRIVATE Qt6::Core)
      endif()
    endif()

    add_test(NAME ${test_name}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${test_name})

    if (UPA_TEST_VALGRIND)
      add_test(NAME ${test_name}_valgrind
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
        COMMAND ${MEMORYCHECK_COMMAND} ${CMAKE_CURRENT_BINARY_DIR}/${test_name})
    endif()
  endforeach()
endif()

# Benchmark targets

if (UPA_BUILD_BENCH)
  file(GLOB bench_files test/bench-*.cpp)

  foreach(file ${bench_files})
    get_filename_component(exe_name ${file} NAME_WE)
    add_executable(${exe_name} ${file})
    target_link_libraries(${exe_name} PRIVATE upa::url)
  endforeach()
endif()

# Fuzzer targets

if (UPA_BUILD_FUZZER)
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
    target_link_libraries(${fuzz_name} ${upa_lib_target})
  endforeach()
endif()

# Example's targets

if (UPA_BUILD_EXAMPLES)
  add_executable(urlparse examples/urlparse.cpp)
  target_link_libraries(urlparse ${upa_lib_target})
endif()

if (UPA_BUILD_EXTRACTED)
  add_subdirectory(examples/extracted-cpp)
endif()

# Install

if (UPA_INSTALL AND NOT UPA_AMALGAMATED)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(
    TARGETS ${upa_lib_target}
    EXPORT ${upa_lib_target}-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  install(
    EXPORT ${upa_lib_target}-targets
    FILE ${upa_lib_name}-targets.cmake
    NAMESPACE upa::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${upa_lib_name}
  )

  # generate the config file that includes the exports
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${upa_lib_name}-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${upa_lib_name}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${upa_lib_name}
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )
  # generate the version file for the config file
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${upa_lib_name}-config-version.cmake
    COMPATIBILITY SameMinorVersion
  )

  # install the generated configuration files
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${upa_lib_name}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${upa_lib_name}-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${upa_lib_name}
  )

  # generate the export targets for the build tree
  # needs to be after the install(TARGETS) command
  export(
    EXPORT ${upa_lib_target}-targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${upa_lib_name}-targets.cmake
    NAMESPACE upa::
  )

  # Packaging
  if (UPA_MAIN_PROJECT)
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_SOURCE_GENERATOR "ZIP")
    set(CPACK_SOURCE_IGNORE_FILES \\.git* /build*/ /*\\.yml)
    set(CPACK_VERBATIM_VARIABLES ON)
    include(CPack)
  endif()
endif()
