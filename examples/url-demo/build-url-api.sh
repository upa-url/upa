#!/bin/sh

# the directory path of this file
# https://stackoverflow.com/q/59895
p="$(dirname "$0")"

# CD to repository root folder
cd $p/../..

# Compile
emcc examples/url-demo/url-api.cpp \
  src/public_suffix_list.cpp \
  src/url.cpp \
  src/url_ip.cpp \
  src/url_search_params.cpp \
  src/url_utf.cpp \
  src/idna.cpp \
  -fwasm-exceptions \
  -Iinclude -std=c++17 -O3 \
  -lembind \
  -o examples/url-demo/www/url-api.js

cd examples/url-demo
