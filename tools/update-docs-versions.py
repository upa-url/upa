#!/usr/bin/env python3
#
# Copyright 2023 Rimas Misevičius
# Distributed under the BSD-style license that can be
# found in the LICENSE file.
import os
import re
import sys

version_regex = r"^v[0-9]+(\.[0-9]+)+$"

def main(versions_folder, git_ref):
    versions_path = os.path.join(versions_folder, "versions.txt")
    if git_ref.startswith("refs/heads/"):
        branch = git_ref[11:]
        if branch == "main":
            update_ver(versions_path, branch, branch)
    elif git_ref.startswith("refs/tags/"):
        version = git_ref[10:]
        if re.match(version_regex, version):
            version_pieces = version.split(".")
            dir = ".".join(version_pieces[:2])
            ver = ".".join(version_pieces)
            update_ver(versions_path, dir, ver)

def update_ver(versions_path, dir, ver):
    dir_ver = dir if dir == ver else dir + ":" + ver
    lines = []
    if os.path.exists(versions_path):
        with open(versions_path, "r") as file:
            lines = file.read().splitlines()
    found = False
    for i in range(len(lines)):
        if lines[i].split(":")[0] == dir:
            lines[i] = dir_ver
            found = True
            break
    if not found:
        lines.append(dir_ver)
    lines.sort(key=key_of_dir_ver)
    # write result
    with open(versions_path, 'w') as file:
        for line in lines:
            file.write(line + '\n')
    print(dir)

def key_of_dir_ver(dir_ver):
    dir = dir_ver.split(":")[0]
    if dir == "main":
        return 0
    # remove starting "v"
    if dir.startswith("v"):
        dir = dir[1:]
    pieces = dir.split(".")
    div = 1000000 * int(pieces[0])
    if len(pieces) > 1:
        div += 1000 * int(pieces[1])
    if len(pieces) > 2:
        div += int(pieces[2])
    return 1 / div

if __name__ == "__main__":
    if len(sys.argv) == 3:
        main(sys.argv[1], sys.argv[2])
    else:
        print("Usage: {} <directory of versions.txt file> <git ref>\n"
            .format(os.path.basename(__file__)),
            file=sys.stderr)
