#!/bin/bash

# the directory path of this file
# https://stackoverflow.com/q/59895
p="$(dirname "$0")"

curl -fsS -o $p/doctest/doctest.h https://raw.githubusercontent.com/onqtam/doctest/2.3.2/doctest/doctest.h
curl -fsS -o $p/picojson/picojson.h https://raw.githubusercontent.com/kazuho/picojson/master/picojson.h
