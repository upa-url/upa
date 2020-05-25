@echo off

REM Download dependances and tests

REM Download submodules
git submodule update --init --recursive

REM Download dependances
call deps/download-deps.bat

REM Download web-platform-tests
call test/download-wpt.bat
