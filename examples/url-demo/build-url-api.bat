@echo off

REM the directory path of this file
set p=%~dp0

REM CD to repository root folder
%~d0
cd %p%../..

REM Compile
call emcc examples/url-demo/url-api.cpp ^
  src/url.cpp ^
  src/url_ip.cpp ^
  src/url_percent_encode.cpp ^
  src/url_search_params.cpp ^
  src/url_utf.cpp ^
  src/idna.cpp ^
  -Iinclude -std=c++17 -O3 ^
  -lembind ^
  -o examples/url-demo/www/url-api.js

cd examples/url-demo
