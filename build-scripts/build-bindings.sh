#!/bin/bash -xe
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# Important note:
# Run from root directory

VERSION=$($(dirname "$(readlink -f "$0")")/version.sh)

function python() {
    # Package python bindings
    PYTHON_BINDINGS_DIR="src/bindings/python/"
    cd $PYTHON_BINDINGS_DIR

    ## Set version and release
    sed \
        -e "s|@VERSION@|${VERSION}|g" \
        < "pyproject.toml.in" \
        > "pyproject.toml"
    
    python3 -m build --wheel --outdir dist/
}

echo "Building bindings $1"
echo ""
$1
