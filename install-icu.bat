@echo off

if "%3" == "" (
  echo install-icu ^<base dir^> ^<version major^> ^<version minor^>
  exit /b 1
)

set BASE_DIR=%1
set VER_MAJOR=%2
set VER_MINOR=%3

if %VER_MAJOR% lss 59 (
  echo Only ICU version 59.1 or later can be installed.
  exit /b 1
)
set MSVC=MSVC2015
if %VER_MAJOR% geq 61 (set MSVC=MSVC2017)
if %VER_MAJOR% geq 68 (set MSVC=MSVC2019)
if %VER_MAJOR% equ 74 if %VER_MINOR% equ 1 (set MSVC=MSVC2022)
if %VER_MAJOR% geq 75 (set MSVC=MSVC2022)

mkdir %BASE_DIR%
curl -fsSL -o %BASE_DIR%\icu4c-bin.zip https://github.com/unicode-org/icu/releases/download/release-%VER_MAJOR%-%VER_MINOR%/icu4c-%VER_MAJOR%_%VER_MINOR%-Win64-%MSVC%.zip
7z x %BASE_DIR%\icu4c-bin.zip -o%BASE_DIR%\ICU
if not exist %BASE_DIR%\ICU\bin64 (
  rename %BASE_DIR%\ICU TMP
  7z x %BASE_DIR%\TMP\*\icu-windows.zip -o%BASE_DIR%\ICU
)
