@echo off

REM the directory path of this file
set p=%~dp0

REM Commit hash of web-platform-tests (wpt)
REM
REM 1. Go to https://github.com/web-platform-tests/wpt/tree/master/url
REM 2. Find "Latest commit" text and click link next to it.
REM 3. Copy hash from URL
set HASH=84782d931516aa13cfe32dc7eaa1377b4d947d66

for %%f in (setters_tests.json toascii.json urltestdata.json percent-encoding.json IdnaTestV2.json) do (
  curl -fsS -o %p%\wpt\%%f https://raw.githubusercontent.com/web-platform-tests/wpt/%HASH%/url/resources/%%f
)
