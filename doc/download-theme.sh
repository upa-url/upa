#!/bin/sh

# the directory path of this file
# https://stackoverflow.com/q/59895
p="$(dirname "$0")"

# The version of Doxygen Awesome theme to download from
# https://github.com/jothepro/doxygen-awesome-css
VERSION=v2.4.1

mkdir -p ${p}/theme

for f in doxygen-awesome.css \
         doxygen-awesome-darkmode-toggle.js \
         doxygen-awesome-paragraph-link.js
do
  curl -fsS -o ${p}/theme/${f} https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/${VERSION}/${f}
done
