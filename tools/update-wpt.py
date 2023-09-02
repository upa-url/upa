#!/usr/bin/env python3
#
# Updates download-wpt.* script files with latest commit
# hash from WPT
#
# Copyright 2023 Rimas Misevičius
# Distributed under the BSD-style license that can be
# found in the LICENSE file.
import json
import re
import urllib.request

# Update file with new HASH value
def update_hash_in_file(file_path, eol, new_hash):
    text = open(file_path, "r").read()
    match = re.search(r"HASH=([0-9a-f]+)", text)
    hash = match.group(1)
    if hash != new_hash:
        text = text[:match.start(1)] + new_hash + text[match.end(1):]
        open(file_path, "w", newline=eol).write(text)
        print("Updated file: " + file_path)

# Use GitHub API to get latest commit hash in the "/url" directory of "web-platform-tests/wpt"
url = "https://api.github.com/repos/web-platform-tests/wpt/commits?sha=master&path=/url&per_page=1"
json_val = json.loads(urllib.request.urlopen(url).read().decode("utf-8"))
new_hash = json_val[0]["sha"]
print("Latest commit hash: " + new_hash)

# Update script files with new HASH value
update_hash_in_file("test/download-wpt.bat", "\r\n", new_hash)
update_hash_in_file("test/download-wpt.sh", "\n", new_hash)
