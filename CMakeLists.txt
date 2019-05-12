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

# Compile test targets

enable_testing()

set(test_files
  test/test-url.cpp
  test/test-url-port.cpp
  test/test-buffer.cpp
  test/test-ipv4.cpp
)
foreach(file ${test_files})
  get_filename_component(test_name ${file} NAME_WLE)

  add_executable(${test_name} ${file})
  target_link_libraries(${test_name} cppurl ICU::i18n ICU::uc)

  add_test(NAME ${test_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${test_name})
endforeach()
