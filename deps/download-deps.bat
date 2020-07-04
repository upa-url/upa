@echo off

REM the directory path of this file
set p=%~dp0

curl -fsS -o %p%\doctest\doctest.h https://raw.githubusercontent.com/onqtam/doctest/2.4.0/doctest/doctest.h
curl -fsS -o %p%\picojson\picojson.h https://raw.githubusercontent.com/kazuho/picojson/master/picojson.h
