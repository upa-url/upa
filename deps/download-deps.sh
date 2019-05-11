#!/bin/bash

# the directory path of this file
# https://stackoverflow.com/q/59895
p="$(dirname "$0")"

curl -o $p/doctest/doctest.h https://raw.githubusercontent.com/onqtam/doctest/2.3.2/doctest/doctest.h
