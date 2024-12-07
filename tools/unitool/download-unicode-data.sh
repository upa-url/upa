#!/bin/sh

# the directory path of this file
# https://stackoverflow.com/q/59895
p="$(dirname "$0")"

# Unicode version
UVER=15.1.0

mkdir -p $p/data

for f in DerivedCoreProperties.txt
do
  curl -fsS -o $p/data/$f https://www.unicode.org/Public/${UVER}/ucd/$f
done
