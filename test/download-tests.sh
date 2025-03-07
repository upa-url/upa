#!/bin/sh

# the directory path of this file
# https://stackoverflow.com/q/59895
p="$(dirname "$0")"

# Commit hash of web-platform-tests (wpt)
#
# Run tools/update-wpt.py to update to the latest commit hash from
# https://github.com/web-platform-tests/wpt/tree/master/url
HASH=d2c39364360cfa39ce900f643f55ca6ef7b33093

for f in setters_tests.json toascii.json urltestdata.json urltestdata-javascript-only.json percent-encoding.json IdnaTestV2.json IdnaTestV2-removed.json
do
  curl -fsS -o $p/wpt/$f https://raw.githubusercontent.com/web-platform-tests/wpt/${HASH}/url/resources/$f
done
