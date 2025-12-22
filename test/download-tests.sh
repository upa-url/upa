#!/bin/sh

# the directory path of this file
# https://stackoverflow.com/q/59895
p="$(dirname "$0")"

# Commit hash of web-platform-tests (wpt)
#
# Run tools/update-wpt.py to update to the latest commit hash from
# https://github.com/web-platform-tests/wpt/tree/master/url
HASH=efb889eb4c9ca0b4e3ebd8ab308646b7483a8f54

for f in setters_tests.json toascii.json urltestdata.json urltestdata-javascript-only.json percent-encoding.json IdnaTestV2.json IdnaTestV2-removed.json
do
  curl -fsS -o $p/wpt/$f https://raw.githubusercontent.com/web-platform-tests/wpt/${HASH}/url/resources/$f
done

# Commit hash of URL Pattern web-platform-tests
# https://github.com/web-platform-tests/wpt/tree/master/urlpattern
URLP_HASH=a2e15ad40518c30c4e7f649584dbda699a40d531

for f in urlpatterntestdata.json
do
  curl -fsS -o $p/wpt/$f https://raw.githubusercontent.com/web-platform-tests/wpt/${URLP_HASH}/urlpattern/resources/$f
done

# Commit hash of the Public Suffix List (PSL)
# https://github.com/publicsuffix/list
PSL_HASH=4a97adbc7f153c31b7d0f62a32bf856f0f1ee6c2

curl -fsS -o $p/psl/public_suffix_list.dat https://raw.githubusercontent.com/publicsuffix/list/${PSL_HASH}/public_suffix_list.dat
curl -fsS -o $p/psl/tests.txt https://raw.githubusercontent.com/publicsuffix/list/${PSL_HASH}/tests/tests.txt
