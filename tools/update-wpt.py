#!/usr/bin/env python3
#
# Update download-tests.* script files with latest commit
# hash from WPT
#
# Copyright 2023-2026 Rimas Misevičius
# Distributed under the BSD-style license that can be
# found in the LICENSE file.
import json
import re
import sys
import urllib.request

# Update file with new HASH value
def update_hash_in_file(file_path, eol, var, new_hash):
    text = open(file_path, "r").read()
    match = re.search(var + r"=([0-9a-f]+)", text)
    hash = match.group(1)
    if hash != new_hash:
        text = text[:match.start(1)] + new_hash + text[match.end(1):]
        open(file_path, "w", newline=eol).write(text)
        print("Updated file: " + file_path)

def get_and_update_hash(name: str, var: str):
    # Use GitHub API to get latest commit hash in the "/<name>" directory of "web-platform-tests/wpt"
    url = f"https://api.github.com/repos/web-platform-tests/wpt/commits?sha=master&path=/{name}&per_page=1"
    json_val = json.loads(urllib.request.urlopen(url).read().decode("utf-8"))
    new_hash = json_val[0]["sha"]
    print("Latest commit hash: " + new_hash)

    # Update script files with new HASH value
    update_hash_in_file("test/download-tests.bat", "\r\n", var, new_hash)
    update_hash_in_file("test/download-tests.sh", "\n", var, new_hash)

def main():
    name2var = {
        "url": "HASH",
        "urlpattern": "URLP_HASH",
    }

    name = None
    var = None
    if len(sys.argv) == 2:
        name = sys.argv[1]
        var = name2var.get(name, None)
    if var is None:
        names = list(name2var.keys())
        print("Choose which WPT commit hash to update:")
        for i, key in enumerate(names):
            print(f" {i + 1}) /{key}")
        answer = input("(1, 2): ")
        try:
            ind = int(answer) - 1
        except ValueError:
            return
        if ind < 0 or ind >= len(names):
            return
        name = names[ind]
        var = name2var.get(name)

    get_and_update_hash(name, var)

if __name__ == "__main__":
    main()
