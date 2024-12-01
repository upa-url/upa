@echo off

REM the directory path of this file
set p=%~dp0

REM Commit hash of web-platform-tests (wpt)
REM
REM Run tools\update-wpt.py to update to the latest commit hash from
REM https://github.com/web-platform-tests/wpt/tree/master/url
set HASH=66adfd2a15d0500978448194cc0333b0906b61dc

for %%f in (setters_tests.json toascii.json urltestdata.json urltestdata-javascript-only.json percent-encoding.json IdnaTestV2.json) do (
  curl -fsS -o %p%\wpt\%%f https://raw.githubusercontent.com/web-platform-tests/wpt/%HASH%/url/resources/%%f
)
