#!/usr/bin/env python3
#
# Copyright 2024 Rimas Misevičius
# Distributed under the BSD-style license that can be
# found in the LICENSE file.
import re

VERSION_NUMBER_COUNT = 3
VERSION_REGEX = "\\.".join(["[0-9]+"] * VERSION_NUMBER_COUNT)

VERSION_HEADER_FILE = "include/upa/url_version.h"
VERSION_MACRO_NAME = "UPA_URL_VERSION"
VERSION_NUM_REGEX = f"#define[ \t]+{VERSION_MACRO_NAME}_([A-Z]+)[ \t]+([0-9]+)"
VERSION_STR_REGEX = f"#define[ \t]+{VERSION_MACRO_NAME}[ \t]+\"({VERSION_REGEX})\""

DOC_FILE = "README.md"
DOC_VERSION_REGEX = f"[v@]({VERSION_REGEX})"

def get_version_str(version: dict):
    return ".".join(map(str, version.values()))

def print_version(text: str, version: dict):
    print(text)
    for n, (key, val) in enumerate(version.items()):
        print(f" {n + 1}) {key}: {val}")

def write_to_file(file_path: str, text: str):
    with open(file_path, "w") as file:
        file.write(text)
    print("Updated file:", file_path)

def update_version_header_file(version: dict):
    new_text = ""
    with open(VERSION_HEADER_FILE, "r") as file:
        for line_nl in file:
            match = re.search(VERSION_NUM_REGEX, line_nl)
            if match:
                line_nl = line_nl[:match.start(2)] + str(version[match.group(1)]) + line_nl[match.end(2):]
            else:
                match = re.search(VERSION_STR_REGEX, line_nl)
                if match:
                    line_nl = line_nl[:match.start(1)] + get_version_str(version) + line_nl[match.end(1):]
            new_text += line_nl
    write_to_file(VERSION_HEADER_FILE, new_text)

def update_documentation(version: dict):
    new_text = ""
    in_cmake = False
    with open(DOC_FILE, "r") as file:
        for line_nl in file:
            # remove ending newline character
            line = line_nl.rstrip()
            if not in_cmake:
                if line == "```cmake":
                    in_cmake = True
            elif line == "```":
                in_cmake = False
            else:
                match = re.search(DOC_VERSION_REGEX, line_nl)
                if match:
                    line_nl = line_nl[:match.start(1)] + get_version_str(version) + line_nl[match.end(1):]
            new_text += line_nl
    write_to_file(DOC_FILE, new_text)

def main():
    version = {}

    print(f"Reading the version from {VERSION_HEADER_FILE}")
    with open(VERSION_HEADER_FILE, "r") as file:
        for line in file:
            match = re.search(VERSION_NUM_REGEX, line)
            if match:
                version[match.group(1)] = int(match.group(2))

    print_version("Current version:", version)
    if len(version) != VERSION_NUMBER_COUNT:
        print(f"Error: version must contain {VERSION_NUMBER_COUNT} numbers.")
        return

    ind = input(f"Which version number to increment? (1...{VERSION_NUMBER_COUNT}): ")
    if ind == "" or int(ind) < 1 or int(ind) > VERSION_NUMBER_COUNT:
        return

    # Update `version` dictionary with new version
    ind = int(ind) - 1
    for n, (key, val) in enumerate(version.items()):
        if n == ind:
            version[key] = val + 1
        elif n > ind:
            version[key] = 0

    print_version("New version:", version)
    if input("Update files with new version? (Y/N): ").upper() == "Y":
        update_version_header_file(version)
        update_documentation(version)

if __name__ == "__main__":
    main()
