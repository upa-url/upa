@echo off

if "%2" == "" (
  echo install-icu ^<base dir^> ^<version^>
  goto :eof
)

set BASE_DIR=%1
set VERSION=%2

mkdir %BASE_DIR%
curl -fsSL -o %BASE_DIR%\icu4c-bin.zip https://github.com/unicode-org/icu/releases/download/release-%VERSION%-1/icu4c-%VERSION%_1-Win64-MSVC2019.zip
if %VERSION% LEQ 71 (
  7z x %BASE_DIR%\icu4c-bin.zip -o%BASE_DIR%\ICU
) ELSE (
  7z x %BASE_DIR%\icu4c-bin.zip -o%BASE_DIR%\TMP
  7z x %BASE_DIR%\TMP\*\icu-windows.zip -o%BASE_DIR%\ICU
)
