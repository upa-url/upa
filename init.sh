#!/bin/sh

# Download dependances and tests

# Download submodules
git submodule update --init --recursive

# Download dependances
deps/download-deps.sh

# Download web-platform-tests
test/download-wpt.sh
