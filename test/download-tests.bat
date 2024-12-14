@echo off

REM the directory path of this file
set p=%~dp0

REM Commit hash of web-platform-tests (wpt)
REM
REM Run tools\update-wpt.py to update to the latest commit hash from
REM https://github.com/web-platform-tests/wpt/tree/master/url
set HASH=efb889eb4c9ca0b4e3ebd8ab308646b7483a8f54

for %%f in (setters_tests.json toascii.json urltestdata.json urltestdata-javascript-only.json percent-encoding.json IdnaTestV2.json IdnaTestV2-removed.json) do (
  curl -fsS -o %p%\wpt\%%f https://raw.githubusercontent.com/web-platform-tests/wpt/%HASH%/url/resources/%%f
)

REM Commit hash of URL Pattern web-platform-tests
REM https://github.com/web-platform-tests/wpt/tree/master/urlpattern
set URLP_HASH=d3e55612911b00cb53271476de610e75a8603ae7

for %%f in (urlpatterntestdata.json) do (
  curl -fsS -o %p%\wpt\%%f https://raw.githubusercontent.com/web-platform-tests/wpt/%URLP_HASH%/urlpattern/resources/%%f
)

REM Commit hash of the Public Suffix List (PSL)
REM https://github.com/publicsuffix/list
set PSL_HASH=4a97adbc7f153c31b7d0f62a32bf856f0f1ee6c2

curl -fsS -o %p%\psl\public_suffix_list.dat https://raw.githubusercontent.com/publicsuffix/list/%PSL_HASH%/public_suffix_list.dat
curl -fsS -o %p%\psl\tests.txt https://raw.githubusercontent.com/publicsuffix/list/%PSL_HASH%/tests/tests.txt
