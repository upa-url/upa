#!/bin/sh

# the directory path of this file
# https://stackoverflow.com/q/59895
p="$(dirname "$0")"

# CD to repository root folder
cd $p/..

# Amalgamate
python3 tools/amalgamate/amalgamate.py -c tools/amalgamate/config-cpp.json -s . -p tools/amalgamate/config-cpp.prologue --no-duplicates
python3 tools/amalgamate/amalgamate.py -c tools/amalgamate/config-h.json -s .

python3 tools/amalgamate/amalgamate.py -c tools/amalgamate/config-psl-cpp.json -s . -p tools/amalgamate/config-psl-cpp.prologue --no-duplicates

# Copy other header files
cp -p include/upa/public_suffix_list.h single_include/upa
cp -p include/upa/url_for_*.h single_include/upa
