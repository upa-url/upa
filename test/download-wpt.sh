#!/bin/sh

# the directory path of this file
# https://stackoverflow.com/q/59895
p="$(dirname "$0")"

# Commit hash of web-platform-tests (wpt)
#
# 1. Go to https://github.com/web-platform-tests/wpt/tree/master/url
# 2. Find "Latest commit" text and click link next to it.
# 3. Copy hash from URL
HASH=c8cd629083f5c9f1d117003b0a12a370a5fdad0e

for f in setters_tests.json toascii.json urltestdata.json
do
  curl -fsS -o $p/wpt/$f https://raw.githubusercontent.com/web-platform-tests/wpt/${HASH}/url/resources/$f
done
