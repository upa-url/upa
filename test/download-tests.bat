@echo off

REM the directory path of this file
set p=%~dp0

REM Commit hash of web-platform-tests (wpt)
REM
REM Run tools\update-wpt.py to update to the latest commit hash from
REM https://github.com/web-platform-tests/wpt/tree/master/url
set HASH=9504a83e014b6b26d6c09eaee89c6b83cb7410c2

for %%f in (setters_tests.json toascii.json urltestdata.json urltestdata-javascript-only.json percent-encoding.json IdnaTestV2.json IdnaTestV2-removed.json) do (
  curl -fsS -o %p%\wpt\%%f https://raw.githubusercontent.com/web-platform-tests/wpt/%HASH%/url/resources/%%f
)

REM Commit hash of the Public Suffix List (PSL)
REM https://github.com/publicsuffix/list
set PSL_HASH=f34b2f1a960238cf765d2e70ca8459ea2de51ec7

curl -fsS -o %p%\psl\public_suffix_list.dat https://raw.githubusercontent.com/publicsuffix/list/%PSL_HASH%/public_suffix_list.dat
curl -fsS -o %p%\psl\tests.txt https://raw.githubusercontent.com/publicsuffix/list/%PSL_HASH%/tests/tests.txt
