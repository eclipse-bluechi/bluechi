#!/bin/bash -xe
# SPDX-License-Identifier: LGPL-2.1-or-later

# Important note: 
# Run from root directory

# Parse package version from the project
meson setup builddir
VERSION="$(meson introspect --projectinfo builddir | jq -r '.version')"

function python() {
    # Package python bindings
    PYTHON_BINDINGS_DIR="src/bindings/python/"
    cd $PYTHON_BINDINGS_DIR

    ## Set version and release
    sed \
        -e "s|@VERSION@|${VERSION}|g" \
        < "setup.py.in" \
        > "setup.py"

    python3 "setup.py" bdist_wheel --dist-dir=dist/
}

echo "Building bindings $1"
echo ""
$1