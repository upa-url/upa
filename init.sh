#!/bin/sh

# Download dependencies and tests

# Download submodules
git submodule update --init --recursive

# Download dependencies
deps/download-deps.sh

# Download web-platform-tests
test/download-tests.sh
