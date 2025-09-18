@echo off

REM the directory path of this file
set p=%~dp0

REM Commit hash of web-platform-tests (wpt)
REM
REM Run tools\update-wpt.py to update to the latest commit hash from
REM https://github.com/web-platform-tests/wpt/tree/master/url
set HASH=40fc257a28faf7c378f59185235685ea8684e8f4

for %%f in (setters_tests.json toascii.json urltestdata.json urltestdata-javascript-only.json percent-encoding.json IdnaTestV2.json IdnaTestV2-removed.json) do (
  curl -fsS -o %p%\wpt\%%f https://raw.githubusercontent.com/web-platform-tests/wpt/%HASH%/url/resources/%%f
)

REM Commit hash of the Public Suffix List (PSL)
REM https://github.com/publicsuffix/list
set PSL_HASH=4a97adbc7f153c31b7d0f62a32bf856f0f1ee6c2

curl -fsS -o %p%\psl\public_suffix_list.dat https://raw.githubusercontent.com/publicsuffix/list/%PSL_HASH%/public_suffix_list.dat
curl -fsS -o %p%\psl\tests.txt https://raw.githubusercontent.com/publicsuffix/list/%PSL_HASH%/tests/tests.txt
