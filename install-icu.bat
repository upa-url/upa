@echo off

if "%3" == "" (
  echo install-icu ^<base dir^> ^<version major^> ^<version minor^>
  goto :eof
)

set BASE_DIR=%1
set VER_MAJOR=%2
set VER_MINOR=%3

mkdir %BASE_DIR%
curl -fsSL -o %BASE_DIR%\icu4c-bin.zip https://github.com/unicode-org/icu/releases/download/release-%VER_MAJOR%-%VER_MINOR%/icu4c-%VER_MAJOR%_%VER_MINOR%-Win64-MSVC2019.zip
7z x %BASE_DIR%\icu4c-bin.zip -o%BASE_DIR%\ICU
if not exist %BASE_DIR%\ICU\bin64 (
  rename %BASE_DIR%\ICU TMP
  7z x %BASE_DIR%\TMP\*\icu-windows.zip -o%BASE_DIR%\ICU
)
