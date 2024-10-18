#!/bin/sh

# the directory path of this file
# https://stackoverflow.com/q/59895
p="$(dirname "$0")"

# CD to repository root folder
cd $p/..

# Amalgamate
python3 tools/amalgamate/amalgamate.py -c tools/amalgamate/config-cpp.json -s . -p tools/amalgamate/config-cpp.prologue --no-duplicates
python3 tools/amalgamate/amalgamate.py -c tools/amalgamate/config-h.json -s .

# Copy url_for_*.h files
cp -p include/upa/url_for_*.h single_include/upa
