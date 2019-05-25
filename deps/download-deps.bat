@echo off

REM the directory path of this file
set p=%~dp0

curl -fsS -o %p%\doctest\doctest.h https://raw.githubusercontent.com/onqtam/doctest/2.3.2/doctest/doctest.h
