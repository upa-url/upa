@echo off

REM Download dependencies and tests

REM Download submodules
git submodule update --init --recursive

REM Download dependencies
call deps/download-deps.bat

REM Download web-platform-tests
call test/download-wpt.bat
