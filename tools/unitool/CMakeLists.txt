cmake_minimum_required(VERSION 3.7...3.13)

# Settings
set(CMAKE_SUPPRESS_REGENERATION true)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project settings
project(unitool LANGUAGES CXX)

add_executable(unitool-id unitool-id.cpp)
