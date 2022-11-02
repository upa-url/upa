@echo off

REM the directory path of this file
set p=%~dp0

curl -fsS -o %p%\doctest\doctest.h https://raw.githubusercontent.com/doctest/doctest/v2.4.9/doctest/doctest.h
curl -fsS -o %p%\picojson\picojson.h https://raw.githubusercontent.com/kazuho/picojson/111c9be5188f7350c2eac9ddaedd8cca3d7bf394/picojson.h
