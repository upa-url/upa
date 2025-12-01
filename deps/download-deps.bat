@echo off

REM the directory path of this file
set p=%~dp0

curl -fsS -o %p%\doctest\doctest.h https://raw.githubusercontent.com/doctest/doctest/v2.4.12/doctest/doctest.h
curl -fsS -o %p%\picojson\picojson.h https://raw.githubusercontent.com/kazuho/picojson/111c9be5188f7350c2eac9ddaedd8cca3d7bf394/picojson.h
curl -fsS -o %p%\ankerl/nanobench.h https://raw.githubusercontent.com/martinus/nanobench/v4.3.11/src/include/nanobench.h
curl -fsS -o %p%\srell\srell.hpp https://raw.githubusercontent.com/upa-url/srell/refs/tags/v4.120/srell-src/single-header/srell.hpp
