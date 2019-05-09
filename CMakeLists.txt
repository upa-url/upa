# FindICU requires 3.7
cmake_minimum_required(VERSION 3.7)

set(CMAKE_SUPPRESS_REGENERATION true)

# use ICU_ROOT
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# Project settings
project(tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

find_package(ICU REQUIRED COMPONENTS i18n uc)

include_directories(
  deps
  src)

add_library(cppurl STATIC
  src/url.cpp
  src/url_idna.cpp
  src/url_ip.cpp
  src/url_percent_encode.cpp
  src/url_utf.cpp)
target_include_directories(cppurl PRIVATE ${ICU_INCLUDE_DIR})

# target test-url
add_executable(test-url test/test-url.cpp)
target_link_libraries(test-url cppurl ICU::i18n ICU::uc)

# Testing

enable_testing()

add_test(NAME test-url
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-url)
